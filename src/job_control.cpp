#include "job_control.h"
#include <algorithm>

std::vector<Job> jobs;
int next_job_id = 1;

void add_job(pid_t pgid, const std::string& command, const std::vector<pid_t>& pids, bool background) {
    Job job;
    job.job_id = next_job_id++;
    job.pgid = pgid;
    job.command = command;
    job.stopped = false;
    job.background = background;
    job.pids = pids;
    jobs.push_back(job);
}

Job* find_job(int job_id) {
    auto it = std::find_if(jobs.begin(), jobs.end(),
        [job_id](const Job& j) { return j.job_id == job_id; });
    
    if (it != jobs.end()) {
        return &(*it);
    }
    return nullptr;
}

void remove_completed_jobs() {
    jobs.erase(std::remove_if(jobs.begin(), jobs.end(),
        [](const Job& j) { return j.pids.empty(); }), jobs.end());
}
