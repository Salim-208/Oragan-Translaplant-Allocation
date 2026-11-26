#include "db.hpp"
#include "matching.hpp"
#include "scheduler.hpp"
#include "allocation.hpp"
<<<<<<< HEAD
#include "process_scheduler.hpp"
=======
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include <fstream>
#include <thread>
<<<<<<< HEAD
#include <cmath>
=======
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc

#ifdef _WIN32
#define getpid _getpid
#define setenv(key, value, overwrite) _putenv_s(key, value)
#endif

struct Demo { const char* organ; const char* blood; int urgency; int wait; };
const Demo DEMO[] = {{"Kidney", "O+", 9, 12}, {"Heart", "A-", 10, 4}, {"Liver", "AB+", 8, 18}, {"Kidney", "A+", 7, 25}, {"Kidney", "O-", 6, 40}, {"Heart", "O+", 8, 16}, {"Liver", "B+", 9, 10}, {"Kidney", "AB+", 7, 31}, {"Heart", "A+", 9, 7}, {"Liver", "O-", 6, 22}, {"Kidney", "B-", 10, 3}, {"Heart", "AB+", 7, 28}, {"Liver", "A-", 8, 19}, {"Kidney", "B+", 6, 35}, {"Heart", "O-", 9, 11}};

<<<<<<< HEAD
Json demoCounts(Db& db) {
    return {
        {"hospitals", db.one("SELECT count(*) AS n FROM hospitals WHERE name LIKE '%(Training)'")["n"]},
        {"donors", db.one("SELECT count(*) AS n FROM donors WHERE name LIKE '%(Training)'")["n"]},
        {"organs", db.one("SELECT count(*) AS n FROM organs o JOIN donors d USING(donor_id) WHERE d.name LIKE '%(Training)'")["n"]},
        {"recipients", db.one("SELECT count(*) AS n FROM recipients WHERE name LIKE '%(Training)'")["n"]},
        {"distances", db.one("SELECT count(*) AS n FROM hospital_distances d JOIN hospitals h ON h.hospital_id=d.from_hospital_id WHERE h.name LIKE '%(Training)'")["n"]}
    };
}

Json seed(Db& db) {
    transaction(db, [&] {
        // This action replaces all operational/sample rows while preserving the
        // current admin account and session. Hospital sessions are discarded.
        db.run("DELETE FROM auth_sessions WHERE role='hospital'");
        db.run("DELETE FROM notifications");
        db.run("DELETE FROM allocation_events");
        db.run("DELETE FROM resource_locks");
        db.run("DELETE FROM allocations");
        db.run("DELETE FROM hospital_distances");
        db.run("DELETE FROM donation_offers");
        db.run("DELETE FROM organ_requests");
        db.run("DELETE FROM organs");
        db.run("DELETE FROM donors");
        db.run("DELETE FROM recipients");
        db.run("DELETE FROM hospital_accounts");
        db.run("DELETE FROM hospitals");
        struct HospitalSeed { const char* name; const char* city; const char* address; const char* phone; double lat, lon; };
        const HospitalSeed hs[] = {
            {"Dehradun Central Transplant Centre (Training)","Dehradun","18 Rajpur Road, Dalanwala","+91 90000 10001",30.3165,78.0322},
            {"Haridwar Riverside Medical Institute (Training)","Haridwar","42 Upper Road, Mayapur","+91 90000 10002",29.9457,78.1642},
            {"Rishikesh Valley Care Hospital (Training)","Rishikesh","7 Veerbhadra Road, Avas Vikas","+91 90000 10003",30.0869,78.2676},
            {"Roorkee Regional Hospital (Training)","Roorkee","25 Civil Lines, Rampur","+91 90000 10004",29.8543,77.8880},
            {"Haldwani Kumaon Medical Centre (Training)","Haldwani","11 Nainital Road, Mukhani","+91 90000 10005",29.2183,79.5130},
            {"Nainital Lakeview Hospital (Training)","Nainital","63 Mallital Road, Mallital","+91 90000 10006",29.3803,79.4636},
            {"Almora Hill Medical Institute (Training)","Almora","9 Mall Road, Dharanaula","+91 90000 10007",29.5971,79.6591},
            {"Pithoragarh Frontier Care Hospital (Training)","Pithoragarh","31 Siltham Road, Siltham","+91 90000 10008",29.5829,80.2182},
            {"Kashipur Terai Regional Centre (Training)","Kashipur","14 Ramnagar Road, Bazpur Road","+91 90000 10009",29.2104,78.9619},
            {"Rudrapur Industrial Medical Centre (Training)","Rudrapur","5 Kichha Road, Awas Vikas","+91 90000 10010",28.9875,79.4141},
            {"Srinagar Garhwal Valley Hospital (Training)","Srinagar Garhwal","22 Badrinath Road, Srinagar","+91 90000 10011",30.2224,78.7833},
            {"Kotdwar Shivalik Medical Centre (Training)","Kotdwar","16 Najibabad Road, Patel Marg","+91 90000 10012",29.7460,78.5250},
            {"Uttarkashi Mountain Care Institute (Training)","Uttarkashi","8 Gangotri Road, Joshiyara","+91 90000 10013",30.7268,78.4354},
            {"Chamoli District Health Centre (Training)","Chamoli","20 Gopeshwar Road, Chamoli","+91 90000 10014",30.4024,79.3206}
        };
        const char* donorNames[] = {"Asha Mehta (Training)","Ravi Kapoor (Training)","Meera Nair (Training)","Arjun Shah (Training)","Priya Desai (Training)","Kabir Malhotra (Training)","Ananya Rao (Training)","Ishaan Verma (Training)","Sara Thomas (Training)","Dev Patel (Training)","Nisha Iyer (Training)","Rohan Banerjee (Training)","Kavya Menon (Training)","Aditya Sethi (Training)"};
        const int donorAges[] = {34,41,29,47,38,52,25,44,31,36,49,27,56,33};
        const char* donorBlood[] = {"O+","A-","B+","AB+","O-","O+","A-","A+","B-","B+","AB-","AB+","O+","A-"};
        const char* donorTissue[] = {"HLA-A02, B07, DR04","HLA-A01, B08, DR03","HLA-A03, B35, DR11","HLA-A02, B44, DR07","HLA-A24, B18, DR15","HLA-A11, B27, DR04","HLA-A02, B51, DR13","HLA-A03, B40, DR08","HLA-A01, B15, DR12","HLA-A26, B38, DR04","HLA-A02, B57, DR07","HLA-A11, B35, DR13","HLA-A24, B44, DR11","HLA-A03, B07, DR15"};
        const char* donorHealth[] = {"Stable donor profile; renal panel within simulated reference range.","No simulated transmissible infection flags; liver panel reviewed.","Cardiac screening marked stable in training record.","Pulmonary assessment and oxygenation marked acceptable.","Renal function stable; medication history reviewed.","Liver enzymes within simulated reference range.","No active infection flags in this fictional profile.","Cardiac imaging recorded as stable in training record.","Kidney function and blood pressure marked acceptable.","Pulmonary screening completed; no simulated contraindication.","Liver profile reviewed; fictional record marked stable.","No active infection flags in simulated donor notes.","Renal panel stable; tissue typing recorded for matching practice.","Cardiac and respiratory screening marked complete."};
        const char* patientNames[] = {"Neha Sharma (Training)","Vikram Joshi (Training)","Fatima Khan (Training)","Arun Kumar (Training)","Pooja Singh (Training)","Rahul Das (Training)","Anjali Gupta (Training)","Sanjay Pillai (Training)","Maya Fernandes (Training)","Karan Bhat (Training)","Divya Reddy (Training)","Imran Ali (Training)","Tara Bose (Training)","Nikhil Jain (Training)"};
        const int patientAges[] = {42,56,31,63,28,49,37,60,45,22,53,34,67,39};
        const char* patientTissue[] = {"HLA-A02, B07, DR04","HLA-A01, B08, DR03","HLA-A03, B35, DR11","HLA-A02, B44, DR07","HLA-A24, B18, DR15","HLA-A11, B27, DR04","HLA-A02, B51, DR13","HLA-A03, B40, DR08","HLA-A01, B15, DR12","HLA-A26, B38, DR04","HLA-A02, B57, DR07","HLA-A11, B35, DR13","HLA-A24, B44, DR11","HLA-A03, B07, DR15"};
        const char* patientCompatibility[] = {"HLA profile available; crossmatch simulated as pending.","ABO compatible candidates only; virtual crossmatch required.","Tissue typing complete; review sensitization before clinical decisions.","No prior transplant recorded in this synthetic profile.","Crossmatch status simulated as pending review.","Antibody screen noted; compatibility requires specialist review.","HLA profile available for academic ranking demonstration.","Virtual crossmatch not yet completed in this training record.","ABO and tissue fields populated for simulation only.","No sensitization history included in this synthetic record.","Crossmatch simulated as pending; verify all clinical data.","HLA profile included; status does not indicate clinical eligibility.","Antibody history marked unknown in this training scenario.","Compatibility details are illustrative and require specialist review."};
        const char* patientMedical[] = {"Chronic kidney disease, stage 5; dialysis profile is simulated.","Dilated cardiomyopathy in fictional case; hemodynamic urgency elevated.","End-stage liver disease; synthetic MELD value 28.","Pulmonary fibrosis in training scenario; oxygen support simulated.","Diabetic nephropathy; dialysis schedule represented for class exercise.","Acute-on-chronic cardiac failure in fictional case profile.","Autoimmune liver disease; simulated MELD value 24.","Inherited renal disease; dialysis duration represented as synthetic.","Cardiomyopathy with recurrent admission history in this scenario.","Chronic liver disease; fictional MELD value 21.","Rapid renal function decline in synthetic case record.","Congenital heart disease; urgency score is for coursework only.","Cholestatic liver disease; fictional case awaiting review.","Bilateral lung disease in synthetic record; oxygen use simulated."};
        const char* demoLogins[] = {"central@hospital.demo","north@hospital.demo","east@hospital.demo","west@hospital.demo"};
        Json hospitals=Json::array();
        for (int i=0;i<14;++i) {
            Json h=db.one("INSERT INTO hospitals(name,city,address,phone,latitude,longitude) VALUES($1,$2,$3,$4,$5,$6) RETURNING hospital_id,name,latitude,longitude",{hs[i].name,hs[i].city,hs[i].address,hs[i].phone,std::to_string(hs[i].lat),std::to_string(hs[i].lon)});
            hospitals.push_back(h);
            if(i<4) db.run("INSERT INTO hospital_accounts(hospital_id,login_id,password_hash) VALUES($1,$2,crypt('Hospital@123',gen_salt('bf')))",{h["hospital_id"].get<std::string>(),demoLogins[i]});
            Json d=db.one("INSERT INTO donors(name,age,blood_type,tissue_type,health_notes,medically_eligible) VALUES($1,$2,$3,$4,$5,TRUE) RETURNING donor_id,name,blood_type",{donorNames[i],std::to_string(donorAges[i]),donorBlood[i],donorTissue[i],donorHealth[i]});
            const std::string organType=DEMO[i].organ;
            const int viabilityHours=18+(i%5)*6;
            Json organ=db.one("INSERT INTO organs(donor_id,organ_type,blood_type,donor_hospital_id,expires_at,viability_notes) VALUES($1,$2,$3,$4,now()+($5||' hours')::interval,$6) RETURNING organ_id",{d["donor_id"].get<std::string>(),organType,donorBlood[i],h["hospital_id"].get<std::string>(),std::to_string(viabilityHours),"Synthetic viability window: "+std::to_string(viabilityHours)+" hours from seed time; clinical viability must be independently established."});
            event(db,"TRAINING_ORGAN_REGISTERED","organ",organ["organ_id"].get<std::string>(),h["hospital_id"].get<std::string>(),{{"organ_type",organType},{"training_data",true}});
            db.run("INSERT INTO recipients(name,age,organ_type,blood_type,tissue_type,compatibility_notes,medical_notes,urgency_score,waiting_days,hospital_id) VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10)",{patientNames[i],std::to_string(patientAges[i]),DEMO[i].organ,DEMO[i].blood,patientTissue[i],patientCompatibility[i],patientMedical[i],std::to_string(DEMO[i].urgency),std::to_string(DEMO[i].wait),h["hospital_id"].get<std::string>()});
        }
        constexpr double pi=3.14159265358979323846;
        for(int i=0;i<14;++i) for(int j=0;j<14;++j) if(i!=j) {
            const double lat1=hs[i].lat*pi/180.0,lat2=hs[j].lat*pi/180.0;
            const double dlat=(hs[j].lat-hs[i].lat)*pi/180.0,dlon=(hs[j].lon-hs[i].lon)*pi/180.0;
            const double a=std::sin(dlat/2)*std::sin(dlat/2)+std::cos(lat1)*std::cos(lat2)*std::sin(dlon/2)*std::sin(dlon/2);
            const double km=6371.0*2*std::atan2(std::sqrt(a),std::sqrt(1-a));
            db.run("INSERT INTO hospital_distances(from_hospital_id,to_hospital_id,distance_km) VALUES($1,$2,$3)",{hospitals[i]["hospital_id"].get<std::string>(),hospitals[j]["hospital_id"].get<std::string>(),std::to_string(km)});
        }
    });
    Json counts = demoCounts(db);
    return {{"done", true}, {"demo_counts", counts}, {"message", "Old operational data was replaced with 14 fictional Uttarakhand training hospitals, donors, organs, recipients, detailed profiles, and a complete state-wide distance matrix."}};
=======
Json seed(Db& db) {
    const int added = std::stoi(demoCount(db));
    if (added >= 15) return {{"done", true}};
    const int total = std::min(added + 3, 15);
    transaction(db, [&] {
        if (db.one("SELECT count(*) AS n FROM hospitals")["n"] == "0") db.run("INSERT INTO hospitals(name) VALUES('Demo City Hospital'),('Demo Metro Hospital'),('Demo Life Care Hospital')");
        const Json h = db.rows("SELECT hospital_id FROM hospitals LIMIT 3");
        for (int i = added; i < total; ++i) {
            char name[32]; std::snprintf(name, sizeof name, "Demo Patient %02d", i + 1);
            db.run("INSERT INTO recipients(name,organ_type,blood_type,urgency_score,waiting_days,hospital_id) VALUES($1,$2,$3,$4,$5,$6)", {name, DEMO[i].organ, DEMO[i].blood, std::to_string(DEMO[i].urgency), std::to_string(DEMO[i].wait), h[i % h.size()]["hospital_id"]});
        }
    });
    return {{"added_count", total - added}};
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
}

Json analytics(Db& db) {
    return db.one("SELECT count(*) FILTER(WHERE status='AVAILABLE') AS available, count(*) FILTER(WHERE status='ALLOCATED') AS allocated, count(*) FILTER(WHERE status='WASTED') AS wasted, count(*) AS total, COALESCE((SELECT round(avg(waiting_days)::numeric,1) FROM allocations a JOIN recipients r USING(recipient_id)),0) AS average_wait_days FROM organs");
}

Json data(Db& db) {
<<<<<<< HEAD
    expire(db); Json stats = analytics(db); stats["demo_records"] = demoCount(db); stats["demo_counts"] = demoCounts(db);
=======
    expire(db); Json stats = analytics(db); stats["demo_records"] = demoCount(db);
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
    return {
        {"hospitals", db.rows("SELECT * FROM hospitals ORDER BY hospital_id")},
        {"donors", db.rows("SELECT * FROM donors ORDER BY donor_id")},
        {"organs", db.rows("SELECT o.*,GREATEST(0,EXTRACT(EPOCH FROM (o.expires_at-now())))::int AS seconds_remaining FROM organs o ORDER BY organ_id")},
        {"recipients", db.rows("SELECT r.*,h.name AS hospital_name FROM recipients r JOIN hospitals h USING(hospital_id) ORDER BY urgency_score DESC,waiting_days DESC")},
        {"allocations", db.rows("SELECT a.*,o.organ_type,r.name AS recipient_name,h.name AS hospital_name FROM allocations a JOIN organs o USING(organ_id) JOIN recipients r USING(recipient_id) JOIN hospitals h ON h.hospital_id=a.hospital_id ORDER BY allocated_at DESC")},
        {"events", db.rows("SELECT event_id,event_type,entity_type,entity_id,hospital_id,payload,created_at FROM allocation_events ORDER BY event_id DESC LIMIT 60")},
<<<<<<< HEAD
        {"requests", db.rows("SELECT q.*,h.name AS hospital_name FROM organ_requests q JOIN hospitals h USING(hospital_id) ORDER BY q.created_at DESC")},
        {"offers", db.rows("SELECT q.*,h.name AS hospital_name FROM donation_offers q JOIN hospitals h USING(hospital_id) ORDER BY q.expires_at")},
        {"notifications", db.rows("SELECT * FROM notifications ORDER BY created_at DESC LIMIT 100")},
=======
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
        {"analytics", stats}};
}

void installRoutes(httplib::Server& srv, Clock::time_point start) {
    auto get = [&](const char* path, Handler h) { srv.Get(path, route(h)); };
    auto post = [&](const char* path, Handler h) { srv.Post(path, route(h)); };
<<<<<<< HEAD
    auto getRole = [&](const char* path, Handler h, const std::string& role) { srv.Get(path, route(h, role)); };
    auto postRole = [&](const char* path, Handler h, const std::string& role) { srv.Post(path, route(h, role)); };
    auto list = [&](const char* path, std::string sql) { get(path, [=](Db& db, const httplib::Request&) { return db.rows(sql); }); };

    srv.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        try { Json b=body(req); Db db; const std::string role=text(b,"role"), login=text(b,"login"), password=text(b,"password"); Json account;
            if(role=="admin") account=db.one("SELECT admin_id FROM admin_accounts WHERE username=$1 AND password_hash=crypt($2,password_hash)",{login,password});
            else if(role=="hospital") account=db.one("SELECT hospital_id FROM hospital_accounts WHERE login_id=$1 AND password_hash=crypt($2,password_hash)",{login,password});
            else throw Error(422,"Select admin or hospital sign in");
            if(account.is_null()) throw Error(401,"Incorrect login or password");
            const std::string id=role=="admin"?account["admin_id"].get<std::string>():account["hospital_id"].get<std::string>();
            Json session=db.one("INSERT INTO auth_sessions(token,role,admin_id,hospital_id) VALUES(gen_random_uuid()::text,$1,NULLIF($2,'')::bigint,NULLIF($3,'')::bigint) RETURNING token",{role,role=="admin"?id:"",role=="hospital"?id:""});
            Json result={{"token",session["token"]},{"role",role}}; if(role=="hospital") result["hospital_id"]=id; send(res,200,result);
        } catch(const Error& e){send(res,e.status,{{"error",e.what()}});} catch(const std::exception& e){send(res,500,{{"error",e.what()}});}
    });
    srv.Post("/api/logout", [](const httplib::Request& req, httplib::Response& res) { try { Db db; Json s=sessionFor(db,req); const std::string auth=req.get_header_value("Authorization"); db.run("DELETE FROM auth_sessions WHERE token=$1",{auth.substr(7)}); send(res,200,{{"logged_out",true}}); } catch(const Error& e){send(res,e.status,{{"error",e.what()}});} });
    getRole("/api/hospital/data", [](Db& db,const httplib::Request& req){
        Json s=sessionFor(db,req); const std::string id=s["hospital_id"].get<std::string>();
        Json transfers=db.rows("SELECT a.allocation_id,a.organ_id,o.organ_type,a.allocated_at,dh.hospital_id AS donor_hospital_id,dh.name AS donor_hospital_name,dh.city AS donor_city,dh.latitude AS donor_latitude,dh.longitude AS donor_longitude,rh.hospital_id AS recipient_hospital_id,rh.name AS recipient_hospital_name,rh.city AS recipient_city,rh.latitude AS recipient_latitude,rh.longitude AS recipient_longitude FROM allocations a JOIN organs o USING(organ_id) JOIN hospitals dh ON dh.hospital_id=o.donor_hospital_id JOIN hospitals rh ON rh.hospital_id=a.hospital_id WHERE a.status='ALLOCATED' AND (a.hospital_id=$1 OR o.donor_hospital_id=$1) ORDER BY a.allocated_at DESC",{id});
        return Json{{"hospital",db.one("SELECT hospital_id,name FROM hospitals WHERE hospital_id=$1",{id})},{"requests",db.rows("SELECT * FROM organ_requests WHERE hospital_id=$1 ORDER BY created_at DESC",{id})},{"offers",db.rows("SELECT * FROM donation_offers WHERE hospital_id=$1 ORDER BY created_at DESC",{id})},{"notifications",db.rows("SELECT * FROM notifications WHERE hospital_id=$1 ORDER BY created_at DESC",{id})},{"transfers",transfers}};
    },"hospital");
    postRole("/api/hospital/requests", [](Db& db,const httplib::Request& req){
        Json b=body(req),s=sessionFor(db,req); const std::string id=s["hospital_id"].get<std::string>(); Json q;
        transaction(db,[&]{
            Json recipient=db.one("INSERT INTO recipients(name,age,organ_type,blood_type,tissue_type,compatibility_notes,medical_notes,urgency_score,hospital_id) VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9) RETURNING recipient_id",
                {text(b,"patient_ref"),number(b,"patient_age",0,120),text(b,"organ_type"),text(b,"blood_type"),text(b,"tissue_type"),b.value("compatibility_notes",std::string()),b.value("medical_notes",std::string()),number(b,"urgency_score",1,10),id});
            q=db.one("INSERT INTO organ_requests(hospital_id,recipient_id,patient_ref,patient_age,organ_type,blood_type,tissue_type,urgency_score,compatibility_notes,medical_notes) VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10) RETURNING *",
                {id,recipient["recipient_id"].get<std::string>(),text(b,"patient_ref"),number(b,"patient_age",0,120),text(b,"organ_type"),text(b,"blood_type"),text(b,"tissue_type"),number(b,"urgency_score",1,10),b.value("compatibility_notes",std::string()),b.value("medical_notes",std::string())});
            event(db,"REQUEST_SUBMITTED","request",q["request_id"].get<std::string>(),id,{{"organ_type",q["organ_type"]}});
        }); return q;
    },"hospital");
    postRole("/api/hospital/offers", [](Db& db,const httplib::Request& req){
        Json b=body(req),s=sessionFor(db,req); const std::string id=s["hospital_id"].get<std::string>(); Json q;
        transaction(db,[&]{
            q=db.one("INSERT INTO donation_offers(hospital_id,donor_ref,organ_type,blood_type,tissue_type,health_notes,available_at,expires_at) VALUES($1,$2,$3,$4,$5,$6,$7::timestamptz,$8::timestamptz) RETURNING *",
                {id,text(b,"donor_ref"),text(b,"organ_type"),text(b,"blood_type"),text(b,"tissue_type"),text(b,"health_notes"),text(b,"available_at"),text(b,"expires_at")});
            event(db,"OFFER_SUBMITTED","offer",q["offer_id"].get<std::string>(),id,{{"organ_type",q["organ_type"]}});
        }); return q;
    },"hospital");

    srv.Get("/api/health", [](const httplib::Request&, httplib::Response& res) { send(res, 200, {{"status", "ok"}}); });
    srv.Post("/api/process-schedule", [](const httplib::Request& req, httplib::Response& res) {
        try { send(res, 200, simulateProcessSchedule(body(req))); }
        catch (const Error& e) { send(res, e.status, {{"error", e.what()}}); }
        catch (const std::exception& e) { send(res, 500, {{"error", e.what()}}); }
    });
    get("/api/status", [start](Db&, const httplib::Request&) { return Json{{"pid", getpid()}, {"state", "RUNNING"}, {"uptime_seconds", std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - start).count()}}; });
=======
    auto list = [&](const char* path, std::string sql) { get(path, [=](Db& db, const httplib::Request&) { return db.rows(sql); }); };

    srv.Get("/api/health", [](const httplib::Request&, httplib::Response& res) { send(res, 200, {{"status", "ok"}}); });
    get("/api/status", [start](Db& db, const httplib::Request&) { return Json{{"pid", getpid()}, {"state", "RUNNING"}, {"uptime_seconds", std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - start).count()}}; });
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
    
    get("/api/data", [](Db& db, const httplib::Request&) { return data(db); });
    get("/api/analytics", [](Db& db, const httplib::Request&) { return analytics(db); });
    list("/api/hospitals", "SELECT * FROM hospitals");
    list("/api/donors", "SELECT * FROM donors");
    get("/api/organs", [](Db& db, const httplib::Request&) { expire(db); return db.rows("SELECT * FROM organs"); });
    list("/api/recipients", "SELECT * FROM recipients");
    list("/api/allocations", "SELECT * FROM allocations");
    list("/api/events", "SELECT * FROM allocation_events LIMIT 100");
<<<<<<< HEAD
    list("/api/requests", "SELECT q.*,h.name AS hospital_name FROM organ_requests q JOIN hospitals h USING(hospital_id) ORDER BY q.created_at DESC");
    list("/api/offers", "SELECT q.*,h.name AS hospital_name FROM donation_offers q JOIN hospitals h USING(hospital_id) ORDER BY q.expires_at");
    list("/api/notifications", "SELECT * FROM notifications ORDER BY created_at DESC LIMIT 100");
=======
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc

    get(R"(/api/organs/(\d+)/matches)", [](Db& db, const httplib::Request& req) { expire(db); return matches(db, std::stoll(req.matches[1])); });
    
    post("/api/hospitals", [](Db& db, const httplib::Request& req) { return db.one("INSERT INTO hospitals(name) VALUES($1) RETURNING *", {text(body(req), "name")}); });
    post("/api/donors", [](Db& db, const httplib::Request& req) { Json b=body(req); return db.one("INSERT INTO donors(name,blood_type,medically_eligible) VALUES($1,$2,$3) RETURNING *", {text(b, "name"), text(b, "blood_type"), b.value("medically_eligible", true) ? "true" : "false"}); });
    post("/api/organs", [](Db& db, const httplib::Request& req) { Json b=body(req); Json hosp = b.value("donor_hospital_id", Json()); return db.one("INSERT INTO organs(donor_id,organ_type,blood_type,donor_hospital_id,expires_at) SELECT donor_id,$2,blood_type,NULLIF($3,'')::bigint,now()+($4||' hours')::interval FROM donors WHERE donor_id=$1 RETURNING *", {number(b, "donor_id", 1, 2147483647), text(b, "organ_type"), hosp.is_null() ? "" : hosp.dump(), std::to_string(b.value("viable_hours", 24.0))}); });
    post("/api/recipients", [](Db& db, const httplib::Request& req) { Json b=body(req); return db.one("INSERT INTO recipients(name,organ_type,blood_type,urgency_score,waiting_days,hospital_id) VALUES($1,$2,$3,$4,$5,$6) RETURNING *", {text(b, "name"), text(b, "organ_type"), text(b, "blood_type"), number(b, "urgency_score", 1, 10), number(b, "waiting_days", 0, 100000), number(b, "hospital_id", 1, 2147483647)}); });
<<<<<<< HEAD
    post("/api/requests", [](Db& db, const httplib::Request& req) { Json b=body(req); const auto h=number(b,"hospital_id",1,2147483647); Json q=db.one("INSERT INTO organ_requests(hospital_id,patient_ref,organ_type,blood_type,urgency_score,compatibility_notes,medical_notes) VALUES($1,$2,$3,$4,$5,$6,$7) RETURNING *", {h,text(b,"patient_ref"),text(b,"organ_type"),text(b,"blood_type"),number(b,"urgency_score",1,10),b.value("compatibility_notes",std::string()),b.value("medical_notes",std::string())}); event(db,"REQUEST_SUBMITTED","request",q["request_id"],h,{{"organ_type",q["organ_type"]}}); return q; });
    post("/api/offers", [](Db& db, const httplib::Request& req) { Json b=body(req); const auto h=number(b,"hospital_id",1,2147483647); Json q=db.one("INSERT INTO donation_offers(hospital_id,donor_ref,organ_type,blood_type,health_notes,available_at,expires_at) VALUES($1,$2,$3,$4,$5,$6::timestamptz,$7::timestamptz) RETURNING *", {h,text(b,"donor_ref"),text(b,"organ_type"),text(b,"blood_type"),b.value("health_notes",std::string()),text(b,"available_at"),text(b,"expires_at")}); event(db,"OFFER_SUBMITTED","offer",q["offer_id"],h,{{"organ_type",q["organ_type"]}}); return q; });
    post("/api/decision", [](Db& db, const httplib::Request& req) -> Json {
        Json b=body(req); const std::string kind=text(b,"kind"), id=number(b,"id",1,2147483647), status=text(b,"status"), reason=b.value("reason",std::string());
        if(status!="ACCEPTED"&&status!="REJECTED"&&status!="PENDING") throw Error(422,"Invalid decision");
        if(status=="REJECTED"&&reason.empty()) throw Error(422,"Rejection reason is required");
        if(kind!="request"&&kind!="offer") throw Error(422,"Invalid queue item");
        Json item;
        transaction(db,[&]{
            if(kind=="request") {
                item=db.one("SELECT request_id,hospital_id,recipient_id,organ_type,blood_type,status FROM organ_requests WHERE request_id=$1 FOR UPDATE",{id});
                if(item.is_null()) throw Error(404,"Request not found");
                if(item["status"]=="ALLOCATED") throw Error(409,"This request is already allocated and cannot be changed");
                if(item["status"]=="ACCEPTED"&&status=="ACCEPTED") throw Error(409,"This request is already approved and is waiting for allocation");
                if(status=="ACCEPTED") {
                    Json offers=db.rows("SELECT o.blood_type FROM organs o JOIN donors d USING(donor_id) WHERE o.status='AVAILABLE' AND o.organ_type=$1 AND o.available_at<=now() AND o.expires_at>now() AND d.medically_eligible=TRUE",{item["organ_type"].get<std::string>()});
                    bool available=false; for(const auto& organ:offers) if(compatible(organ["blood_type"].get<std::string>(),item["blood_type"].get<std::string>())) { available=true; break; }
                    if(!available) throw Error(409,"No currently available, unexpired, medically eligible organ matches this request. Keep it pending until an offer is verified.");
                }
                db.run("UPDATE organ_requests SET status=$1,decision_reason=NULLIF($2,''),updated_at=now() WHERE request_id=$3",{status,reason,id});
                if(!item["recipient_id"].is_null()) db.run("UPDATE recipients SET active=$1 WHERE recipient_id=$2",{status=="REJECTED"?"false":"true",item["recipient_id"].get<std::string>()});
            } else {
                item=db.one("SELECT offer_id,hospital_id,donor_ref,organ_type,blood_type,tissue_type,health_notes,available_at,expires_at,status FROM donation_offers WHERE offer_id=$1 FOR UPDATE",{id});
                if(item.is_null()) throw Error(404,"Donation offer not found");
                if(item["status"]=="ALLOCATED") throw Error(409,"This offer is already allocated and cannot be changed");
                if(item["status"]=="ACCEPTED") throw Error(409,"This offer is already verified and entered into inventory");
                if(status=="ACCEPTED") {
                    if(item["expires_at"].get<std::string>()=="") throw Error(409,"Offer has no valid expiry");
                    Json validity=db.one("SELECT ($1::timestamptz > now()) AS not_expired,($2::timestamptz < $3::timestamptz) AS valid_window",{item["expires_at"].get<std::string>(),item["available_at"].get<std::string>(),item["expires_at"].get<std::string>()});
                    if(!validity["not_expired"].get<bool>()||!validity["valid_window"].get<bool>()) throw Error(409,"Offer is expired or has an invalid availability window");
                    Json donor=db.one("INSERT INTO donors(name,blood_type,tissue_type,health_notes,medically_eligible) VALUES($1,$2,$3,$4,TRUE) RETURNING donor_id",{item["donor_ref"].get<std::string>(),item["blood_type"].get<std::string>(),item["tissue_type"].get<std::string>(),item["health_notes"].get<std::string>()});
                    Json organ=db.one("INSERT INTO organs(donor_id,organ_type,blood_type,donor_hospital_id,available_at,expires_at,viability_notes) VALUES($1,$2,$3,$4,$5::timestamptz,$6::timestamptz,$7) RETURNING organ_id",
                        {donor["donor_id"].get<std::string>(),item["organ_type"].get<std::string>(),item["blood_type"].get<std::string>(),item["hospital_id"].get<std::string>(),item["available_at"].get<std::string>(),item["expires_at"].get<std::string>(),item["health_notes"].get<std::string>()});
                    db.run("UPDATE donation_offers SET status='ACCEPTED',decision_reason=NULL,organ_id=$1 WHERE offer_id=$2",{organ["organ_id"].get<std::string>(),id});
                    item["organ_id"]=organ["organ_id"];
                } else db.run("UPDATE donation_offers SET status=$1,decision_reason=NULLIF($2,'') WHERE offer_id=$3",{status,reason,id});
            }
            const std::string hospital=item["hospital_id"].get<std::string>();
            const std::string msg="Your "+kind+" "+publicId(kind=="request"?"REQ":"OFF",id)+" is "+(status=="ACCEPTED"&&kind=="offer"?"accepted and entered into the available-organ queue":status=="ACCEPTED"?"approved; allocation remains subject to admin matching and approval":status)+"."+(reason.empty()?"":" Reason: "+reason);
            db.run("INSERT INTO notifications(hospital_id,title,message) VALUES($1,$2,$3)",{hospital,kind+" decision",msg});
            event(db,kind=="request"?"REQUEST_DECIDED":"OFFER_DECIDED",kind,id,hospital,{{"status",status},{"reason",reason},{"organ_id",item.value("organ_id",Json())}});
        });
        return Json{{"updated",true},{"status",status},{"organ_id",item.value("organ_id",Json())}};
    });
    srv.Put("/api/distances", route([](Db& db, const httplib::Request& req) {
        Json b = body(req);
        const auto from = number(b, "from_hospital_id", 1, 2147483647);
        const auto to = number(b, "to_hospital_id", 1, 2147483647);
        if (!b.contains("distance_km") || !b["distance_km"].is_number() || !std::isfinite(b["distance_km"].get<double>()) || b["distance_km"].get<double>() < 0)
            throw Error(422, "Invalid: distance_km");
        return db.one("INSERT INTO hospital_distances(from_hospital_id,to_hospital_id,distance_km) VALUES($1,$2,$3) ON CONFLICT(from_hospital_id,to_hospital_id) DO UPDATE SET distance_km=EXCLUDED.distance_km RETURNING *",
            {from, to, std::to_string(b["distance_km"].get<double>())});
    }));
    
    post("/api/claim", [](Db& db, const httplib::Request& req) { expire(db); return claim(db, body(req)); });
    post("/api/auto-allocate", [](Db& db, const httplib::Request& req) { expire(db); return autoAllocate(db, body(req)); });
=======
    
    post("/api/claim", [](Db& db, const httplib::Request& req) { expire(db); return claim(db, body(req)); });
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
    post("/api/reject", [](Db& db, const httplib::Request& req) { Json b=body(req); event(db, "HOSPITAL_REJECTED", "organ", number(b, "organ_id", 1, 2147483647), number(b, "hospital_id", 1, 2147483647), {{"reason", text(b, "reason")}}); return Json{{"recorded", true}}; });
    post("/api/deadlines", [](Db& db, const httplib::Request&) { return expire(db); });
    post("/api/deadlock", [](Db& db, const httplib::Request&) { return deadlockDemo(db); });
    post("/api/crash", [](Db& db, const httplib::Request&) { return simulateCrash(db); });
    post("/api/recover", [](Db& db, const httplib::Request&) { return recover(db); });
    post("/api/seed", [](Db& db, const httplib::Request&) { return seed(db); });
}

int main(int argc, char** argv) {
<<<<<<< HEAD
    int port = 8000;
    if (argc == 3 && std::string(argv[1]) == "--port") {
        try { port = std::stoi(argv[2]); } catch (...) { std::cerr << "Invalid port: " << argv[2] << '\n'; return 2; }
    } else if (argc != 1) {
        std::cerr << "Usage: organ_matching.exe [--port PORT]\n";
        return 2;
    }
    if (port < 1 || port > 65535) { std::cerr << "Port must be between 1 and 65535\n"; return 2; }
    if (!std::getenv("DATABASE_URL")) setenv("DATABASE_URL", "postgresql://postgres:postgres@127.0.0.1:5433/organ_allocation", 1);
=======
    if (!std::getenv("DATABASE_URL")) setenv("DATABASE_URL", "postgresql://postgres:postgres@localhost:5433/organ_allocation", 1);
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
    
    try {
        std::ifstream schema("schema.sql");
        std::stringstream sql; sql << schema.rdbuf();
        Db db; db.script(sql.str()); expire(db);
    } catch (const std::exception& e) { std::cerr << "DB Error: " << e.what() << '\n'; return 1; }

    httplib::Server server;
    server.set_mount_point("/", "web");
    installRoutes(server, Clock::now());

    std::atomic<bool> stop{false};
    std::thread deadlineWorker([&] {
        while (!stop) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (!stop) { try { Db db; expire(db); } catch (...) {} }
        }
    });

<<<<<<< HEAD
    std::cout << "Server running on port " << port << '\n';
    bool listening = server.listen("127.0.0.1", port);
    stop = true; deadlineWorker.join();
    return listening ? 0 : 1;
}
=======
    std::cout << "Server running on port 8000\n";
    bool listening = server.listen("127.0.0.1", 8000);
    stop = true; deadlineWorker.join();
    return listening ? 0 : 1;
}
>>>>>>> c8c39cd3f83ae965361b95e09e525a0ecec1b0dc
