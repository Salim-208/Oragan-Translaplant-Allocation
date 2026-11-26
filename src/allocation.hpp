#pragma once
#include "db.hpp"
#include "matching.hpp"
#include "process_scheduler.hpp"

// [DBMS CONCEPT: Mutual Exclusion / Locking]
inline Json claimLocked(Db& db, const std::string& organId, const std::string& recipientId,
                        const std::string& reason, bool skipIneligible = false) {
    Json organ = db.one("SELECT o.organ_type,o.blood_type,o.status,o.expires_at<=now() AS expired,d.medically_eligible FROM organs o JOIN donors d USING(donor_id) WHERE organ_id=$1 FOR UPDATE", {organId});
    if (organ.is_null() || organ["status"] != "AVAILABLE" || organ["expired"].get<bool>()) throw Error(409, "Organ unavailable");
    if (!organ["medically_eligible"].get<bool>()) throw Error(409, "Donor is medically ineligible");

    Json recipient = db.one("SELECT organ_type,blood_type,hospital_id,active,(SELECT status FROM organ_requests WHERE recipient_id=$1 ORDER BY request_id DESC LIMIT 1) AS request_status FROM recipients WHERE recipient_id=$1 FOR UPDATE", {recipientId});
    const bool recipientEligible = !recipient.is_null() && recipient["active"].get<bool>() &&
        recipient["organ_type"] == organ["organ_type"] && compatible(organ["blood_type"], recipient["blood_type"]) &&
        (recipient["request_status"].is_null() || recipient["request_status"] == "ACCEPTED");
    const bool alreadyAllocated = !recipient.is_null() && !db.one("SELECT allocation_id FROM allocations WHERE recipient_id=$1 AND status='ALLOCATED' LIMIT 1", {recipientId}).is_null();
    if (!recipientEligible || alreadyAllocated) {
        if (skipIneligible) return Json();
        throw Error(409, "Recipient ineligible or already allocated");
    }

    std::string hosp = recipient["hospital_id"];
    db.run("INSERT INTO resource_locks(organ_id,hospital_id) VALUES($1,$2)", {organId, hosp});
    db.run("INSERT INTO allocations(organ_id,recipient_id,hospital_id,reason) VALUES($1,$2,$3,$4)", {organId, recipientId, hosp, reason});
    db.run("UPDATE organs SET status='ALLOCATED' WHERE organ_id=$1", {organId});
    db.run("DELETE FROM resource_locks WHERE organ_id=$1", {organId});
    event(db, "ALLOCATED", "organ", organId, hosp, {{"recipient_id", recipientId}});
    Json offer = db.one("UPDATE donation_offers SET status='ALLOCATED' WHERE organ_id=$1 AND status='ACCEPTED' RETURNING offer_id,hospital_id",{organId});
    if(!offer.is_null()) {
        db.run("INSERT INTO notifications(hospital_id,title,message) VALUES($1,'Donation offer allocated',$2)",{offer["hospital_id"].get<std::string>(),"Donation offer "+publicId("OFF",offer["offer_id"].get<std::string>())+" has been allocated by the admin."});
        event(db,"OFFER_ALLOCATED","offer",offer["offer_id"].get<std::string>(),offer["hospital_id"].get<std::string>(),{{"organ_id",organId},{"recipient_hospital_id",hosp}});
    }
    Json request = db.one("UPDATE organ_requests SET status='ALLOCATED',decision_reason='Allocated through admin approval',updated_at=now() WHERE recipient_id=$1 AND status='ACCEPTED' RETURNING request_id,hospital_id",{recipientId});
    if(!request.is_null()) {
        db.run("INSERT INTO notifications(hospital_id,title,message) VALUES($1,'Allocation approved',$2)",{request["hospital_id"].get<std::string>(),"Request "+publicId("REQ",request["request_id"].get<std::string>())+" has been allocated after admin matching."});
        event(db,"REQUEST_ALLOCATED","request",request["request_id"].get<std::string>(),request["hospital_id"].get<std::string>(),{{"organ_id",organId}});
    }
    return {{"allocated", true}, {"organ_id", organId}, {"recipient_id", recipientId}};
}

inline Json claim(Db& db, const Json& in) {
    const std::string organId = number(in, "organ_id", 1, 2147483647);
    const std::string recipientId = number(in, "recipient_id", 1, 2147483647);
    Json result;
    transaction(db, [&] { result = claimLocked(db, organId, recipientId, "Manually allocated"); });
    return result;
}

inline Json autoAllocate(Db& db, const Json& in) {
    const std::string organId = number(in, "organ_id", 1, 2147483647);
    Json result;
    transaction(db, [&] {
        Json lock = db.one("SELECT organ_id FROM organs WHERE organ_id=$1 FOR UPDATE", {organId});
        if (lock.is_null()) throw Error(409, "Organ unavailable");
        Json ranked = matches(db, std::stoll(organId))["matches"];
        if (ranked.empty()) throw Error(409, "No eligible recipients available for this organ");

        // Treat every eligible recipient request as a ready process. Higher urgency maps
        // to a smaller OS priority number; matching rank breaks equal-urgency ties.
        Json jobs = Json::array();
        for (std::size_t i = 0; i < ranked.size(); ++i)
            jobs.push_back({{"id", "P" + std::to_string(i + 1)}, {"arrival_time", 0}, {"burst_time", 1},
                            {"priority", 11 - static_cast<int>(num(ranked[i], "urgency_score"))}});
        Json dispatch = simulateProcessSchedule({{"algorithm", "PRIORITY"}, {"jobs", jobs}});
        Json selected;
        std::string selectedProcess;
        for (const auto& slice : dispatch["timeline"]) {
            selectedProcess = slice["process"].get<std::string>();
            const std::size_t selectedIndex = static_cast<std::size_t>(std::stoi(selectedProcess.substr(1)) - 1);
            const Json& candidate = ranked[selectedIndex];
            const std::string recipientId = candidate["recipient_id"].get<std::string>();
            result = claimLocked(db, organId, recipientId, "Allocated by OS Priority Scheduler", true);
            if (!result.is_null()) { selected = candidate; break; }
        }
        if (result.is_null()) throw Error(409, "Eligible recipients changed while the scheduler was running; retry");
        const std::string recipientId = selected["recipient_id"].get<std::string>();
        const int osPriority = 11 - static_cast<int>(num(selected, "urgency_score"));
        event(db, "PROCESS_SCHEDULED", "organ", organId, selected["hospital_id"].get<std::string>(),
              {{"process_id", selectedProcess}, {"algorithm", "PRIORITY"},
               {"os_priority", osPriority}, {"recipient_id", recipientId}, {"matching_score", selected["priority_score"]}});
        result["recipient_name"] = selected["name"];
        result["priority_score"] = selected["priority_score"];
        result["matching_rank"] = selected["rank"];
        result["process_id"] = selectedProcess;
        result["os_priority"] = osPriority;
        result["algorithm"] = "PRIORITY";
    });
    return result;
}

// [OS CONCEPT: Deadlock Avoidance/Resolution]
inline Json deadlockDemo(Db& db) {
    Json cycle = Json::array({"Hospital A waits for Hospital B", "Hospital B waits for Hospital A"});
    event(db, "DEADLOCK_DETECTED", "system", "", "", {{"cycle", cycle}, {"victim", "Hospital B"}});
    return {{"deadlock_detected", true}, {"cycle", cycle}, {"victim", "Hospital B"}, {"resolution", "Abort Hospital B and release lock"}};
}

// [DBMS CONCEPT: Crash Recovery]
inline Json simulateCrash(Db& db) {
    transaction(db, [&] {
        db.run("DELETE FROM allocations"); db.run("UPDATE organs SET status='AVAILABLE' WHERE status='ALLOCATED'");
        event(db, "CRASH_SIMULATED", "system", "", "");
    });
    return {{"status", "crashed"}};
}

inline Json recover(Db& db) {
    transaction(db, [&] {
        for (const auto& e : db.rows("SELECT entity_id,hospital_id,payload FROM allocation_events WHERE event_type='ALLOCATED' ORDER BY event_id")) {
            db.run("INSERT INTO allocations(organ_id,recipient_id,hospital_id,reason) VALUES($1,$2,$3,'Recovered') ON CONFLICT DO NOTHING", {e["entity_id"].get<std::string>(), e["payload"]["recipient_id"].get<std::string>(), e["hospital_id"].get<std::string>()});
            db.run("UPDATE organs SET status='ALLOCATED' WHERE organ_id=$1", {e["entity_id"].get<std::string>()});
        }
        event(db, "RECOVERY_COMPLETED", "system", "", "");
    });
    return {{"status", "recovered"}};
}
