#pragma once
#include "db.hpp"
#include <algorithm>

// [OS CONCEPT: Priority Scheduling Algorithm]
inline bool compatible(const std::string& d, const std::string& r) {
    if (d.size() < 2 || r.size() < 2) return false;
    auto abo = [](const std::string& t) { return t.substr(0, t.size() - 1); };
    return (abo(d) == "O" || abo(r) == "AB" || abo(d) == abo(r)) && (d.back() == '-' || r.back() == '+');
}

inline Json matches(Db& db, long long organId) {
    Json organ = db.one("SELECT o.organ_id,o.organ_type,o.blood_type,o.donor_hospital_id,o.status,o.available_at,o.expires_at,o.expires_at<=now() AS expired,o.available_at>now() AS not_yet_available,d.medically_eligible,d.tissue_type AS donor_tissue_type FROM organs o JOIN donors d USING(donor_id) WHERE organ_id=$1", {std::to_string(organId)});
    if (organ.is_null() || organ["status"] != "AVAILABLE" || organ["expired"].get<bool>() || organ["not_yet_available"].get<bool>() || !organ["medically_eligible"].get<bool>()) throw Error(409, "Organ is unavailable, not yet available, expired, or medically ineligible");

    Json candidates = db.rows(
        "SELECT r.recipient_id,r.name,r.age,r.organ_type,r.blood_type,r.tissue_type,r.compatibility_notes,r.medical_notes,r.urgency_score,r.waiting_days,r.hospital_id,h.name AS hospital_name, "
        "COALESCE((SELECT distance_km FROM hospital_distances d WHERE d.from_hospital_id=$1 AND d.to_hospital_id=r.hospital_id),0) AS distance_km "
        "FROM recipients r JOIN hospitals h USING(hospital_id) WHERE r.active=TRUE AND r.organ_type=$2 "
        "AND (NOT EXISTS (SELECT 1 FROM organ_requests q WHERE q.recipient_id=r.recipient_id) OR EXISTS (SELECT 1 FROM organ_requests q WHERE q.recipient_id=r.recipient_id AND q.status='ACCEPTED')) "
        "AND NOT EXISTS (SELECT 1 FROM allocations a WHERE a.recipient_id=r.recipient_id AND a.status='ALLOCATED')",
        {idText(organ["donor_hospital_id"], "0"), organ["organ_type"]});

    Json eligible = Json::array();
    for (auto& r : candidates) {
        if (!compatible(organ["blood_type"], r["blood_type"])) continue;
        const std::string donorTissue=organ.value("donor_tissue_type",std::string());
        const std::string recipientTissue=r.value("tissue_type",std::string());
        const bool tissueProfileMatch=!donorTissue.empty()&&!recipientTissue.empty()&&donorTissue==recipientTissue;
        r["tissue_profile_match"] = tissueProfileMatch;
        // Academic ranking only: exact synthetic tissue-profile equality gets +20.
        r["priority_score"] = num(r, "urgency_score") * 10 + num(r, "waiting_days") * 2 - num(r, "distance_km") + (tissueProfileMatch ? 20 : 0);
        eligible.push_back(r);
    }
    std::sort(eligible.begin(), eligible.end(), [](const Json& a, const Json& b) {
        if (a["priority_score"] != b["priority_score"]) return a["priority_score"].get<double>() > b["priority_score"].get<double>();
        if (a["urgency_score"] != b["urgency_score"]) return num(a, "urgency_score") > num(b, "urgency_score");
        if (a["waiting_days"] != b["waiting_days"]) return num(a, "waiting_days") > num(b, "waiting_days");
        if (a["distance_km"] != b["distance_km"]) return num(a, "distance_km") < num(b, "distance_km");
        return num(a, "recipient_id") < num(b, "recipient_id");
    });
    for (std::size_t i = 0; i < eligible.size(); ++i) eligible[i]["rank"] = i + 1;
    return {{"organ", organ}, {"matches", eligible}};
}
