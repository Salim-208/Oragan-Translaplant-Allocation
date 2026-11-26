#pragma once
#include "db.hpp"
#include <algorithm>
#include <deque>
#include <limits>
#include <unordered_set>

struct ProcessJob {
    std::string id;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int completion = 0;
    std::size_t order;
};

struct ScheduleSlice { std::string process; int start; int end; };

inline Json simulateProcessSchedule(const Json& input) {
    const std::string algorithm = text(input, "algorithm");
    if (algorithm != "FCFS" && algorithm != "SJF" && algorithm != "PRIORITY" && algorithm != "RR")
        throw Error(422, "Algorithm must be FCFS, SJF, PRIORITY, or RR");
    if (!input.contains("jobs") || !input["jobs"].is_array() || input["jobs"].empty() || input["jobs"].size() > 1000)
        throw Error(422, "Provide between 1 and 1000 processes");

    int quantum = 1;
    if (algorithm == "RR") {
        if (!input.contains("quantum") || !input["quantum"].is_number_integer()) throw Error(422, "Round Robin requires an integer quantum");
        quantum = input["quantum"].get<int>();
        if (quantum < 1 || quantum > 1000) throw Error(422, "Quantum must be between 1 and 1000");
    }

    std::vector<ProcessJob> jobs;
    std::unordered_set<std::string> ids;
    for (std::size_t i = 0; i < input["jobs"].size(); ++i) {
        const Json& job = input["jobs"][i];
        const std::string id = text(job, "id");
        if (id.size() > 12 || !ids.insert(id).second) throw Error(422, "Process IDs must be unique and at most 12 characters");
        auto integer = [&](const char* key, int minimum, int maximum) {
            if (!job.contains(key) || !job[key].is_number_integer()) throw Error(422, "Invalid process field: " + std::string(key));
            const int value = job[key].get<int>();
            if (value < minimum || value > maximum) throw Error(422, "Out of range: " + std::string(key));
            return value;
        };
        const int arrival = integer("arrival_time", 0, 10000);
        const int burst = integer("burst_time", 1, 10000);
        const int priority = integer("priority", 1, 10);
        jobs.push_back({id, arrival, burst, priority, burst, 0, i});
    }

    std::vector<ScheduleSlice> timeline;
    int time = 0;
    auto append = [&](std::size_t i, int duration) {
        timeline.push_back({jobs[i].id, time, time + duration});
        time += duration;
        jobs[i].remaining -= duration;
        if (jobs[i].remaining == 0) jobs[i].completion = time;
    };
    auto byArrival = [&](std::size_t a, std::size_t b) {
        if (jobs[a].arrival != jobs[b].arrival) return jobs[a].arrival < jobs[b].arrival;
        return jobs[a].order < jobs[b].order;
    };

    if (algorithm == "RR") {
        std::vector<std::size_t> arrivals(jobs.size());
        for (std::size_t i = 0; i < jobs.size(); ++i) arrivals[i] = i;
        std::sort(arrivals.begin(), arrivals.end(), byArrival);
        std::deque<std::size_t> ready;
        std::size_t next = 0;
        while (next < arrivals.size() || !ready.empty()) {
            if (ready.empty() && time < jobs[arrivals[next]].arrival) time = jobs[arrivals[next]].arrival;
            while (next < arrivals.size() && jobs[arrivals[next]].arrival <= time) ready.push_back(arrivals[next++]);
            const std::size_t current = ready.front(); ready.pop_front();
            append(current, std::min(quantum, jobs[current].remaining));
            while (next < arrivals.size() && jobs[arrivals[next]].arrival <= time) ready.push_back(arrivals[next++]);
            if (jobs[current].remaining > 0) ready.push_back(current);
        }
    } else {
        std::vector<bool> complete(jobs.size(), false);
        std::size_t done = 0;
        while (done < jobs.size()) {
            std::size_t best = jobs.size();
            for (std::size_t i = 0; i < jobs.size(); ++i) {
                if (complete[i] || jobs[i].arrival > time) continue;
                if (best == jobs.size()) { best = i; continue; }
                bool prefer = false;
                if (algorithm == "SJF" && jobs[i].burst != jobs[best].burst) prefer = jobs[i].burst < jobs[best].burst;
                else if (algorithm == "PRIORITY" && jobs[i].priority != jobs[best].priority) prefer = jobs[i].priority < jobs[best].priority;
                else if (jobs[i].arrival != jobs[best].arrival) prefer = jobs[i].arrival < jobs[best].arrival;
                else prefer = jobs[i].order < jobs[best].order;
                if (prefer) best = i;
            }
            if (best == jobs.size()) {
                for (std::size_t i = 0; i < jobs.size(); ++i)
                    if (!complete[i] && (best == jobs.size() || jobs[i].arrival < jobs[best].arrival)) best = i;
                time = jobs[best].arrival;
                continue;
            }
            append(best, jobs[best].remaining);
            complete[best] = true;
            ++done;
        }
    }

    Json results = Json::array();
    double waitingTotal = 0;
    double turnaroundTotal = 0;
    for (const auto& job : jobs) {
        const int turnaround = job.completion - job.arrival;
        const int waiting = turnaround - job.burst;
        waitingTotal += waiting;
        turnaroundTotal += turnaround;
        results.push_back({{"id", job.id}, {"arrival_time", job.arrival}, {"burst_time", job.burst},
                           {"priority", job.priority}, {"completion_time", job.completion},
                           {"waiting_time", waiting}, {"turnaround_time", turnaround}});
    }
    Json slices = Json::array();
    for (const auto& slice : timeline) slices.push_back({{"process", slice.process}, {"start", slice.start}, {"end", slice.end}});
    return {{"algorithm", algorithm}, {"quantum", algorithm == "RR" ? Json(quantum) : Json()},
            {"timeline", slices}, {"results", results},
            {"average_waiting_time", waitingTotal / jobs.size()},
            {"average_turnaround_time", turnaroundTotal / jobs.size()}};
}
