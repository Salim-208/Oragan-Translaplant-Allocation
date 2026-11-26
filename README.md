# Organ Allocation Queue — Academic Simulation

A small C++17 and PostgreSQL teaching app for organ matching, hospital queue submissions, allocation decisions, and OS/DBMS concepts. Names are realistic but entirely fictional training records. This is not a clinical decision system and must not be used for real patient care.

## Features

- Central queue for hospital organ requests and donation offers.
- Admin decision controls: accept, reject with a required reason, or return to pending.
- Decision notifications and append-only allocation events.
- Admin and hospital sign-in with PostgreSQL bcrypt password hashes and server-checked sessions.
- Hospital-scoped dashboard endpoints; hospital submissions bind to the signed-in hospital.
- Pending hospital requests stay out of the match queue until an admin approves them; approval is blocked if no eligible, unexpired ABO-compatible organ is available.
- Admin verification of a donation offer creates a real inventory row that is then subject to the same matching and deadline scheduler.
- Allocation updates the request and donation-offer workflow state and writes notifications and audit events in the same transaction.
- ABO/Rh compatibility, urgency and waiting-time ranking, and transport-distance tie breaking.
- Matching queue includes approved hospital submissions, with a transparent academic exact-tissue-profile bonus.
- Uttarakhand-only admin route planner shows the 14 fictional state training hospitals on an OpenStreetMap map and requests road distance/drive-time estimates from OSRM.
- Hospital users see a donor-to-recipient route only after an approved allocation involves their hospital.
- Atomic allocation with row locks, one-allocation-per-organ constraint, and viability deadlines.
- OS/DBMS demonstrations: priority/deadline scheduling, mutual exclusion, deadlock, transactions, and log recovery.
- Responsive dashboard with synthetic demo data.
- Rich fictional training profiles with hospital addresses and approximate coordinates, donor and recipient ages, tissue profiles, medical notes, viability details, and a full inter-hospital distance matrix.
- Admin-only dataset replacement action that clears all operational records and seeds the new training dataset in one transaction.

## Run on Windows

Requirements: Docker Desktop and MSYS2 UCRT64 `g++` with PostgreSQL 16 client headers at `C:\msys64\ucrt64\opt\pg-16`.

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\ucrt64\opt\pg-16\bin;$env:Path"
$env:DATABASE_URL = "postgresql://postgres:postgres@127.0.0.1:5433/organ_allocation"
docker compose up -d postgres
if (!(Test-Path .env)) { Copy-Item .env.example .env }
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
.\build-nocmake\organ_matching.exe --port 8000
```

Open <http://127.0.0.1:8000>. Demo accounts:

| Role | Login | Password |
| --- | --- | --- |
| Admin | `admin@organ.demo` | `Admin@123` |
| Dehradun Central Transplant Centre | `central@hospital.demo` | `Hospital@123` |
| Haridwar Riverside Medical Institute | `north@hospital.demo` | `Hospital@123` |
| Rishikesh Valley Care Hospital | `east@hospital.demo` | `Hospital@123` |
| Roorkee Regional Hospital | `west@hospital.demo` | `Hospital@123` |

Sign in as admin and use **Replace all data with training dataset** to delete existing hospitals, donors, recipients, organs, requests, offers, allocations, notifications, distances, and audit events, then seed 14 fictional Uttarakhand hospitals and records of each core type. The current admin account/session remains active; hospital accounts are recreated. The operation is transactional. A hospital session only receives its own requests, offers, notifications, and authorized transfer routes. Admin and hospital dashboards both have a **Sign out** button. The admin queue supports decisions; rejection requires a reason and notifies the hospital. Data refreshes every 10 seconds.

## Important limitations

This version is a local academic simulation, not a production clinical service. Routes use approximate training city-centre coordinates and public OpenStreetMap/OSRM services; travel time is not live traffic, and no vehicle GPS is tracked. Internet and public-service availability are required for map tiles and routing. Use synthetic data only. Allocation scores are educational examples and do not represent applicable medical allocation rules. Change or replace the demo credentials before exposing the service beyond localhost.

## Project files

- `src/main.cpp`: HTTP API, seeded simulation data, queue submissions, decisions, and notifications.
- `src/matching.hpp` / `src/allocation.hpp`: compatibility, ranking, and safe allocation transaction.
- `src/process_scheduler.hpp`: CPU scheduling demonstration.
- `schema.sql`: PostgreSQL schema, constraints, and indexes.
- `web/`: dashboard interface.
- `build.ps1`: Windows build script.
