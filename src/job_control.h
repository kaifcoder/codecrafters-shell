#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <vector>
#include <string>
#include <unistd.h>

struct Job {
    int job_id;
    pid_t pgid;
    std::string command;
    bool stopped;
    bool background;
    std::vector<pid_t> pids;
};

extern std::vector<Job> jobs;
extern int next_job_id;

void add_job(pid_t pgid, const std::string& command, const std::vector<pid_t>& pids, bool background);
Job* find_job(int job_id);
void remove_completed_jobs();

#endif // JOB_CONTROL_H
