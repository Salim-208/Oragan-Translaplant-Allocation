#pragma once
#include "db.hpp"

// [OS CONCEPT: Deadline Scheduler] Wastes organs past viability time.
inline Json expire(Db& db) {
    Json ids = Json::array();
    transaction(db, [&] {
        for (const auto& row : db.rows("UPDATE organs SET status='WASTED' WHERE status='AVAILABLE' AND expires_at<=now() RETURNING organ_id,donor_hospital_id")) {
            event(db, "ORGAN_EXPIRED", "organ", idText(row["organ_id"]), idText(row["donor_hospital_id"]));
            ids.push_back(row["organ_id"]);
        }
    });
    return {{"expired_count", ids.size()}, {"organ_ids", ids}};
}
