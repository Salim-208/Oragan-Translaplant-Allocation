# Simple ER Diagram — Organ Allocation System

`PK` = primary key · `FK` = foreign key · `UK` = unique key

```mermaid
erDiagram
    HOSPITALS {
        bigint hospital_id PK
        text name UK
        text city
        text address
        text phone
        double latitude
        double longitude
        timestamptz created_at
    }
    ADMIN_ACCOUNTS {
        bigint admin_id PK
        text username UK
        text password_hash
    }
    HOSPITAL_ACCOUNTS {
        bigint account_id PK
        bigint hospital_id FK_UK
        text login_id UK
        text password_hash
    }
    AUTH_SESSIONS {
        text token PK
        text role
        bigint admin_id FK
        bigint hospital_id FK
        timestamptz created_at
    }
    DONORS {
        bigint donor_id PK
        text name
        smallint age
        text blood_type
        text tissue_type
        text health_notes
        boolean medically_eligible
        timestamptz created_at
    }
    ORGANS {
        bigint organ_id PK
        bigint donor_id FK
        text organ_type
        text blood_type
        bigint donor_hospital_id FK
        timestamptz available_at
        timestamptz expires_at
        text viability_notes
        text status
    }
    RECIPIENTS {
        bigint recipient_id PK
        text name
        smallint age
        text organ_type
        text blood_type
        text tissue_type
        text compatibility_notes
        text medical_notes
        smallint urgency_score
        integer waiting_days
        bigint hospital_id FK
        boolean active
        timestamptz created_at
    }
    ORGAN_REQUESTS {
        bigint request_id PK
        bigint hospital_id FK
        bigint recipient_id FK
        text patient_ref
        smallint patient_age
        text organ_type
        text blood_type
        text tissue_type
        smallint urgency_score
        text compatibility_notes
        text medical_notes
        text status
        text decision_reason
        timestamptz created_at
        timestamptz updated_at
    }
    DONATION_OFFERS {
        bigint offer_id PK
        bigint hospital_id FK
        bigint organ_id FK
        text donor_ref
        text organ_type
        text blood_type
        text tissue_type
        text health_notes
        timestamptz available_at
        timestamptz expires_at
        text status
        text decision_reason
        timestamptz created_at
    }
    ALLOCATIONS {
        bigint allocation_id PK
        bigint organ_id FK_UK
        bigint recipient_id FK
        bigint hospital_id FK
        text status
        text reason
        timestamptz allocated_at
    }
    RESOURCE_LOCKS {
        bigint organ_id PK_FK
        bigint hospital_id FK
        timestamptz locked_at
    }
    HOSPITAL_DISTANCES {
        bigint from_hospital_id PK_FK
        bigint to_hospital_id PK_FK
        numeric distance_km
    }
    NOTIFICATIONS {
        bigint notification_id PK
        bigint hospital_id FK
        text title
        text message
        timestamptz read_at
        timestamptz created_at
    }
    ALLOCATION_EVENTS {
        bigint event_id PK
        text event_type
        text entity_type
        bigint entity_id
        bigint hospital_id FK
        jsonb payload
        timestamptz created_at
    }

    HOSPITALS ||--o| HOSPITAL_ACCOUNTS : has_login
    ADMIN_ACCOUNTS ||--o{ AUTH_SESSIONS : creates
    HOSPITALS ||--o{ AUTH_SESSIONS : creates
    DONORS ||--o{ ORGANS : donates
    HOSPITALS o|--o{ ORGANS : donor_location
    HOSPITALS ||--o{ RECIPIENTS : registers
    HOSPITALS ||--o{ ORGAN_REQUESTS : submits
    RECIPIENTS o|--o{ ORGAN_REQUESTS : referenced_by
    HOSPITALS ||--o{ DONATION_OFFERS : submits
    ORGANS o|--o{ DONATION_OFFERS : linked_after_approval
    ORGANS ||--o| ALLOCATIONS : allocated_once
    RECIPIENTS ||--o{ ALLOCATIONS : receives
    HOSPITALS ||--o{ ALLOCATIONS : recipient_hospital
    ORGANS ||--o| RESOURCE_LOCKS : locked_once
    HOSPITALS ||--o{ RESOURCE_LOCKS : lock_owner
    HOSPITALS ||--o{ HOSPITAL_DISTANCES : distance_from
    HOSPITALS ||--o{ HOSPITAL_DISTANCES : distance_to
    HOSPITALS ||--o{ NOTIFICATIONS : receives
    HOSPITALS o|--o{ ALLOCATION_EVENTS : event_hospital
```

## Relationship summary

- One hospital can have zero or one hospital login and can register many recipients, requests, offers, notifications, and allocations.
- One donor can provide multiple organ records. An organ can be linked to at most one allocation because `allocations.organ_id` is unique.
- An organ request belongs to one hospital and can optionally refer to a recipient. A donation offer belongs to one hospital and can optionally link to an organ after approval.
- A recipient can have multiple allocation records over time; each allocation points to one organ, one recipient, and one recipient hospital.
- Hospital distance has a composite primary key: `(from_hospital_id, to_hospital_id)`.
- `AUTH_SESSIONS` belongs to either an admin account or a hospital account depending on `role`.

This is the current database design in `schema.sql`. Training hospitals are in Uttarakhand. Live vehicle locations are not stored in a separate table in this version.
