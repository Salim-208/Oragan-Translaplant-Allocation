CREATE TABLE IF NOT EXISTS hospitals (
<<<<<<< HEAD
  hospital_id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL UNIQUE,
  city TEXT NOT NULL DEFAULT '',
  address TEXT NOT NULL DEFAULT '',
  phone TEXT NOT NULL DEFAULT '',
  latitude DOUBLE PRECISION,
  longitude DOUBLE PRECISION,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
ALTER TABLE hospitals ADD COLUMN IF NOT EXISTS city TEXT NOT NULL DEFAULT '';
ALTER TABLE hospitals ADD COLUMN IF NOT EXISTS address TEXT NOT NULL DEFAULT '';
ALTER TABLE hospitals ADD COLUMN IF NOT EXISTS phone TEXT NOT NULL DEFAULT '';
ALTER TABLE hospitals ADD COLUMN IF NOT EXISTS latitude DOUBLE PRECISION;
ALTER TABLE hospitals ADD COLUMN IF NOT EXISTS longitude DOUBLE PRECISION;
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE TABLE IF NOT EXISTS admin_accounts (
  admin_id BIGSERIAL PRIMARY KEY,
  username TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS hospital_accounts (
  account_id BIGSERIAL PRIMARY KEY,
  hospital_id BIGINT NOT NULL UNIQUE REFERENCES hospitals(hospital_id) ON DELETE CASCADE,
  login_id TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS auth_sessions (
  token TEXT PRIMARY KEY,
  role TEXT NOT NULL CHECK (role IN ('admin','hospital')),
  admin_id BIGINT REFERENCES admin_accounts(admin_id) ON DELETE CASCADE,
  hospital_id BIGINT REFERENCES hospitals(hospital_id) ON DELETE CASCADE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  CHECK ((role='admin' AND admin_id IS NOT NULL AND hospital_id IS NULL) OR (role='hospital' AND hospital_id IS NOT NULL AND admin_id IS NULL))
);
INSERT INTO admin_accounts(username,password_hash) VALUES ('admin@organ.demo',crypt('Admin@123',gen_salt('bf'))) ON CONFLICT(username) DO NOTHING;

CREATE TABLE IF NOT EXISTS donors (
  donor_id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  age SMALLINT CHECK (age BETWEEN 18 AND 100),
  blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
  tissue_type TEXT NOT NULL DEFAULT '',
  health_notes TEXT NOT NULL DEFAULT '',
  medically_eligible BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
ALTER TABLE donors ADD COLUMN IF NOT EXISTS medically_eligible BOOLEAN NOT NULL DEFAULT TRUE;
ALTER TABLE donors ADD COLUMN IF NOT EXISTS age SMALLINT;
ALTER TABLE donors ADD COLUMN IF NOT EXISTS tissue_type TEXT NOT NULL DEFAULT '';
ALTER TABLE donors ADD COLUMN IF NOT EXISTS health_notes TEXT NOT NULL DEFAULT '';

CREATE TABLE IF NOT EXISTS organs (
  organ_id BIGSERIAL PRIMARY KEY,
  donor_id BIGINT NOT NULL REFERENCES donors(donor_id),
  organ_type TEXT NOT NULL,
  blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
  donor_hospital_id BIGINT REFERENCES hospitals(hospital_id),
  available_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  expires_at TIMESTAMPTZ NOT NULL,
  viability_notes TEXT NOT NULL DEFAULT '',
  status TEXT NOT NULL DEFAULT 'AVAILABLE' CHECK (status IN ('AVAILABLE','ALLOCATED','WASTED')),
  CHECK (expires_at > available_at)
);
ALTER TABLE organs ADD COLUMN IF NOT EXISTS viability_notes TEXT NOT NULL DEFAULT '';

CREATE TABLE IF NOT EXISTS recipients (
  recipient_id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  age SMALLINT CHECK (age BETWEEN 0 AND 120),
  organ_type TEXT NOT NULL,
  blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
  tissue_type TEXT NOT NULL DEFAULT '',
  compatibility_notes TEXT NOT NULL DEFAULT '',
  medical_notes TEXT NOT NULL DEFAULT '',
  urgency_score SMALLINT NOT NULL CHECK (urgency_score BETWEEN 1 AND 10),
  waiting_days INTEGER NOT NULL DEFAULT 0 CHECK (waiting_days >= 0),
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  active BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
ALTER TABLE recipients ADD COLUMN IF NOT EXISTS age SMALLINT;
ALTER TABLE recipients ADD COLUMN IF NOT EXISTS tissue_type TEXT NOT NULL DEFAULT '';
ALTER TABLE recipients ADD COLUMN IF NOT EXISTS compatibility_notes TEXT NOT NULL DEFAULT '';
ALTER TABLE recipients ADD COLUMN IF NOT EXISTS medical_notes TEXT NOT NULL DEFAULT '';

-- Rename records from earlier teaching seeds to realistic fictional names.
UPDATE hospitals SET name = CASE name
  WHEN 'Demo Hospital Central' THEN 'City Central Medical Center (Training)'
  WHEN 'Demo Hospital North' THEN 'North Valley Hospital (Training)'
  WHEN 'Demo Hospital East' THEN 'Riverside Specialty Hospital (Training)'
  WHEN 'Demo Hospital West' THEN 'Westlake University Hospital (Training)'
  ELSE regexp_replace(name, '^Demo Hospital ([0-9]+)$', 'Regional Transplant Center \1 (Training)') END
WHERE name LIKE 'Demo Hospital %';
UPDATE donors SET name = CASE name
  WHEN 'Demo Donor Asha' THEN 'Asha Mehta (Training)' WHEN 'Demo Donor Ravi' THEN 'Ravi Kapoor (Training)'
  WHEN 'Demo Donor Meera' THEN 'Meera Nair (Training)' WHEN 'Demo Donor Arjun' THEN 'Arjun Shah (Training)'
  WHEN 'Demo Donor 05' THEN 'Priya Desai (Training)' WHEN 'Demo Donor 06' THEN 'Kabir Malhotra (Training)'
  WHEN 'Demo Donor 07' THEN 'Ananya Rao (Training)' WHEN 'Demo Donor 08' THEN 'Ishaan Verma (Training)'
  WHEN 'Demo Donor 09' THEN 'Sara Thomas (Training)' WHEN 'Demo Donor 10' THEN 'Dev Patel (Training)'
  WHEN 'Demo Donor 11' THEN 'Nisha Iyer (Training)' WHEN 'Demo Donor 12' THEN 'Rohan Banerjee (Training)'
  WHEN 'Demo Donor 13' THEN 'Kavya Menon (Training)' WHEN 'Demo Donor 14' THEN 'Aditya Sethi (Training)' ELSE name END
WHERE name LIKE 'Demo Donor %';
UPDATE recipients SET name = CASE name
  WHEN 'Demo Patient 01' THEN 'Neha Sharma (Training)' WHEN 'Demo Patient 02' THEN 'Vikram Joshi (Training)'
  WHEN 'Demo Patient 03' THEN 'Fatima Khan (Training)' WHEN 'Demo Patient 04' THEN 'Arun Kumar (Training)'
  WHEN 'Demo Patient Extra 01' THEN 'Pooja Singh (Training)' WHEN 'Demo Patient Extra 02' THEN 'Rahul Das (Training)'
  WHEN 'Demo Patient Extra 03' THEN 'Anjali Gupta (Training)' WHEN 'Demo Patient Extra 04' THEN 'Sanjay Pillai (Training)'
  WHEN 'Demo Patient Extra 05' THEN 'Maya Fernandes (Training)' WHEN 'Demo Patient Extra 06' THEN 'Karan Bhat (Training)'
  WHEN 'Demo Patient Extra 07' THEN 'Divya Reddy (Training)' WHEN 'Demo Patient Extra 08' THEN 'Imran Ali (Training)'
  WHEN 'Demo Patient Extra 09' THEN 'Tara Bose (Training)' WHEN 'Demo Patient Extra 10' THEN 'Nikhil Jain (Training)' ELSE name END
WHERE name LIKE 'Demo Patient %';

CREATE TABLE IF NOT EXISTS hospital_distances (
  from_hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  to_hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  distance_km NUMERIC(10,2) NOT NULL CHECK (distance_km >= 0),
  PRIMARY KEY (from_hospital_id, to_hospital_id)
);

CREATE TABLE IF NOT EXISTS allocations (
  allocation_id BIGSERIAL PRIMARY KEY,
  organ_id BIGINT NOT NULL UNIQUE REFERENCES organs(organ_id),
  recipient_id BIGINT NOT NULL REFERENCES recipients(recipient_id),
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  status TEXT NOT NULL DEFAULT 'ALLOCATED',
  reason TEXT NOT NULL,
  allocated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS resource_locks (
  organ_id BIGINT PRIMARY KEY REFERENCES organs(organ_id),
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  locked_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS allocation_events (
  event_id BIGSERIAL PRIMARY KEY,
  event_type TEXT NOT NULL,
  entity_type TEXT NOT NULL,
  entity_id BIGINT,
  hospital_id BIGINT REFERENCES hospitals(hospital_id),
  payload JSONB NOT NULL DEFAULT '{}'::jsonb,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Hospital-submitted queue items. Patient/donor details remain attached to the
-- submitting hospital; admin views are centralized for this academic demo.
CREATE TABLE IF NOT EXISTS organ_requests (
  request_id BIGSERIAL PRIMARY KEY,
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  recipient_id BIGINT REFERENCES recipients(recipient_id),
  patient_ref TEXT NOT NULL,
  patient_age SMALLINT CHECK (patient_age BETWEEN 0 AND 120),
  organ_type TEXT NOT NULL,
  blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
  tissue_type TEXT NOT NULL DEFAULT '',
  urgency_score SMALLINT NOT NULL CHECK (urgency_score BETWEEN 1 AND 10),
  compatibility_notes TEXT NOT NULL DEFAULT '',
  medical_notes TEXT NOT NULL DEFAULT '',
  status TEXT NOT NULL DEFAULT 'PENDING' CHECK (status IN ('PENDING','ACCEPTED','REJECTED','ALLOCATED')),
  decision_reason TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
ALTER TABLE organ_requests ADD COLUMN IF NOT EXISTS recipient_id BIGINT REFERENCES recipients(recipient_id);
ALTER TABLE organ_requests ADD COLUMN IF NOT EXISTS patient_age SMALLINT;
ALTER TABLE organ_requests ADD COLUMN IF NOT EXISTS tissue_type TEXT NOT NULL DEFAULT '';
ALTER TABLE organ_requests DROP CONSTRAINT IF EXISTS organ_requests_status_check;
ALTER TABLE organ_requests ADD CONSTRAINT organ_requests_status_check CHECK (status IN ('PENDING','ACCEPTED','REJECTED','ALLOCATED'));
CREATE TABLE IF NOT EXISTS donation_offers (
  offer_id BIGSERIAL PRIMARY KEY,
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  organ_id BIGINT REFERENCES organs(organ_id),
  donor_ref TEXT NOT NULL,
  organ_type TEXT NOT NULL,
  blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
  tissue_type TEXT NOT NULL DEFAULT '',
  health_notes TEXT NOT NULL DEFAULT '',
  available_at TIMESTAMPTZ NOT NULL,
  expires_at TIMESTAMPTZ NOT NULL,
  status TEXT NOT NULL DEFAULT 'PENDING' CHECK (status IN ('PENDING','ACCEPTED','REJECTED','ALLOCATED')),
  decision_reason TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  CHECK (expires_at > available_at)
);
ALTER TABLE donation_offers ADD COLUMN IF NOT EXISTS organ_id BIGINT REFERENCES organs(organ_id);
ALTER TABLE donation_offers ADD COLUMN IF NOT EXISTS tissue_type TEXT NOT NULL DEFAULT '';
CREATE TABLE IF NOT EXISTS notifications (
  notification_id BIGSERIAL PRIMARY KEY,
  hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
  title TEXT NOT NULL,
  message TEXT NOT NULL,
  read_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_recipients_match_priority ON recipients(organ_type, blood_type, urgency_score DESC, waiting_days DESC);
CREATE INDEX IF NOT EXISTS idx_organs_available_deadline ON organs(status, expires_at);
CREATE INDEX IF NOT EXISTS idx_events_entity_time ON allocation_events(entity_type, entity_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_requests_queue ON organ_requests(status, organ_type, blood_type, urgency_score DESC, created_at);
CREATE INDEX IF NOT EXISTS idx_offers_queue ON donation_offers(status, organ_type, blood_type, expires_at);
CREATE INDEX IF NOT EXISTS idx_notifications_hospital ON notifications(hospital_id, created_at DESC);

-- Repeatable demo accounts; synthetic-only credentials for local coursework.
INSERT INTO hospital_accounts(hospital_id,login_id,password_hash)
SELECT hospital_id, login_id, crypt('Hospital@123',gen_salt('bf'))
FROM (VALUES ('City Central Medical Center (Training)','central@hospital.demo'),('North Valley Hospital (Training)','north@hospital.demo'),('Riverside Specialty Hospital (Training)','east@hospital.demo'),('Westlake University Hospital (Training)','west@hospital.demo')) AS demo(name,login_id)
JOIN hospitals USING(name)
ON CONFLICT (hospital_id) DO NOTHING;
=======
    hospital_id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS donors (
    donor_id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS organs (
    organ_id BIGSERIAL PRIMARY KEY,
    donor_id BIGINT NOT NULL REFERENCES donors(donor_id),
    organ_type TEXT NOT NULL,
    blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
    donor_hospital_id BIGINT REFERENCES hospitals(hospital_id),
    available_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    expires_at TIMESTAMPTZ NOT NULL,
    status TEXT NOT NULL DEFAULT 'AVAILABLE' CHECK (status IN ('AVAILABLE','ALLOCATED','WASTED')),
    CHECK (expires_at > available_at)
);
CREATE TABLE IF NOT EXISTS recipients (
    recipient_id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    organ_type TEXT NOT NULL,
    blood_type TEXT NOT NULL CHECK (blood_type IN ('O-','O+','A-','A+','B-','B+','AB-','AB+')),
    urgency_score SMALLINT NOT NULL CHECK (urgency_score BETWEEN 1 AND 10),
    waiting_days INTEGER NOT NULL DEFAULT 0 CHECK (waiting_days >= 0),
    hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
    active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS hospital_distances (
    from_hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
    to_hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
    distance_km NUMERIC(10,2) NOT NULL CHECK (distance_km >= 0),
    PRIMARY KEY (from_hospital_id, to_hospital_id)
);
CREATE TABLE IF NOT EXISTS allocations (
    allocation_id BIGSERIAL PRIMARY KEY,
    organ_id BIGINT NOT NULL UNIQUE REFERENCES organs(organ_id),
    recipient_id BIGINT NOT NULL REFERENCES recipients(recipient_id),
    hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
    status TEXT NOT NULL CHECK (status IN ('ALLOCATED','REJECTED')),
    reason TEXT NOT NULL,
    allocated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS resource_locks (
    organ_id BIGINT PRIMARY KEY REFERENCES organs(organ_id) ON DELETE CASCADE,
    hospital_id BIGINT NOT NULL REFERENCES hospitals(hospital_id),
    acquired_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE TABLE IF NOT EXISTS allocation_events (
    event_id BIGSERIAL PRIMARY KEY,
    event_type TEXT NOT NULL,
    entity_type TEXT NOT NULL,
    entity_id BIGINT,
    hospital_id BIGINT REFERENCES hospitals(hospital_id),
    payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_recipients_match_priority
    ON recipients (organ_type, blood_type, urgency_score DESC);
CREATE INDEX IF NOT EXISTS idx_organs_available_deadline
    ON organs (status, expires_at) WHERE status = 'AVAILABLE';
CREATE INDEX IF NOT EXISTS idx_events_entity_time ON allocation_events (entity_type, entity_id, event_id);
CREATE INDEX IF NOT EXISTS idx_allocations_time ON allocations (allocated_at DESC);

-- Enforce append-only audit history for application roles. The table owner can still
-- maintain the schema; normal API operations only insert and select event rows.
CREATE OR REPLACE FUNCTION reject_allocation_event_mutation() RETURNS trigger AS $$
BEGIN
    RAISE EXCEPTION 'allocation_events is append-only';
END;
$$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS allocation_events_append_only ON allocation_events;
CREATE TRIGGER allocation_events_append_only
    BEFORE UPDATE OR DELETE ON allocation_events
    FOR EACH ROW EXECUTE FUNCTION reject_allocation_event_mutation();
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
