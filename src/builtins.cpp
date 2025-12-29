#include "builtins.h"
#include "utils.h"
#include "job_control.h"
#include "shell.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <readline/history.h>
#include <signal.h>

std::map<std::string, builtin_func> builtins;
std::map<std::string, int> last_written_positions;

extern pid_t shell_pgid;
extern bool shell_is_interactive;

void init_builtins() {
    builtins["exit"] = exit_command;
    builtins["echo"] = echo_command;
    builtins["type"] = type_command;
    builtins["pwd"] = pwd_command;
    builtins["cd"] = cd_command;
    builtins["history"] = history_command;
    builtins["fg"] = fg_command;
    builtins["bg"] = bg_command;
    builtins["jobs"] = jobs_command;
    builtins["help"] = help_command;
}

bool is_builtin(const std::string& cmd) {
    return builtins.find(cmd) != builtins.end();
}

void exit_command(const std::vector<std::string>& args) {
    int code = 0;
    if (!args.empty()) {
        try {
            code = std::stoi(args[0]);
        } catch (...) {
            code = 0;
        }
    }
    std::exit(code);
}

void echo_command(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        std::cout << args[i];
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
}

void type_command(const std::vector<std::string>& args) {
    if (args.empty()) return;
    
    std::string cmd = args[0];
    
    if (builtins.find(cmd) != builtins.end()) {
        std::cout << cmd << " is a shell builtin" << std::endl;
        return;
    }
    
    std::string executable_path = find_executable_in_path(cmd);
    if (!executable_path.empty()) {
        std::cout << cmd << " is " << executable_path << std::endl;
    } else {
        std::cout << cmd << ": not found" << std::endl;
    }
}

void pwd_command(const std::vector<std::string>&) {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) {
        std::cout << cwd << std::endl;
    }
}

void cd_command(const std::vector<std::string>& args) {
    std::string target_dir;
    
    if (args.empty()) {
        const char* home = std::getenv("HOME");
        target_dir = home ? home : ".";
    } else if (args[0] == "~") {
        const char* home = std::getenv("HOME");
        target_dir = home ? home : ".";
    } else if (args[0] == "-") {
        const char* oldpwd = std::getenv("OLDPWD");
        target_dir = oldpwd ? oldpwd : ".";
    } else {
        target_dir = args[0];
    }
    
    char old_pwd[4096];
    if (getcwd(old_pwd, sizeof(old_pwd))) {
        if (chdir(target_dir.c_str()) == 0) {
            setenv("OLDPWD", old_pwd, 1);
        } else {
            std::cout << "cd: " << target_dir << ": No such file or directory" << std::endl;
        }
    }
}

void history_command(const std::vector<std::string>& args) {
    if (args.size() >= 2 && args[0] == "-r") {
        std::string history_file = args[1];
        std::ifstream file(history_file);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    add_history(line.c_str());
                }
            }
            file.close();
        } else {
            std::cout << "history: " << history_file << ": No such file or directory" << std::endl;
        }
        return;
    }
    
    if (args.size() >= 2 && args[0] == "-w") {
        std::string history_file = args[1];
        std::ofstream file(history_file);
        if (file.is_open()) {
            HISTORY_STATE* state = history_get_history_state();
            for (int i = 0; i < state->length; i++) {
                HIST_ENTRY* entry = history_get(i);
                if (entry) {
                    file << entry->line << std::endl;
                }
            }
            file.close();
            free(state);
        } else {
            std::cout << "history: " << history_file << ": Error writing file" << std::endl;
        }
        return;
    }
    
    if (args.size() >= 2 && args[0] == "-a") {
        std::string history_file = args[1];
        HISTORY_STATE* state = history_get_history_state();
        int current_length = state->length;
        int last_written = last_written_positions[history_file];
        
        std::ofstream file(history_file, std::ios::app);
        if (file.is_open()) {
            for (int i = last_written; i < current_length; i++) {
                HIST_ENTRY* entry = history_get(i);
                if (entry) {
                    file << entry->line << std::endl;
                }
            }
            file.close();
            last_written_positions[history_file] = current_length;
        } else {
            std::cout << "history: " << history_file << ": Error writing file" << std::endl;
        }
        free(state);
        return;
    }
    
    HISTORY_STATE* state = history_get_history_state();
    int start = 0;
    
    if (!args.empty()) {
        try {
            int requested_limit = std::stoi(args[0]);
            start = std::max(0, state->length - requested_limit);
        } catch (...) {
            std::cout << "history: " << args[0] << ": numeric argument required" << std::endl;
            free(state);
            return;
        }
    }
    
    for (int i = start; i < state->length; i++) {
        HIST_ENTRY* entry = history_get(i);
        if (entry) {
            std::cout << "    " << (i + 1) << "  " << entry->line << std::endl;
        }
    }
    free(state);
}

void fg_command(const std::vector<std::string>& args) {
    int job_id = jobs.empty() ? -1 : jobs.back().job_id;
    
    if (!args.empty()) {
        try {
            job_id = std::stoi(args[0]);
        } catch (...) {
            std::cout << "fg: " << args[0] << ": no such job" << std::endl;
            return;
        }
    }
    
    Job* job = find_job(job_id);
    if (!job) {
        std::cout << "fg: " << job_id << ": no such job" << std::endl;
        return;
    }
    
    std::cout << job->command << std::endl;
    
    if (job->stopped) {
        kill(-job->pgid, SIGCONT);
        job->stopped = false;
    }
    
    tcsetpgrp(STDIN_FILENO, job->pgid);
    job->background = false;
    
    int status;
    for (pid_t pid : job->pids) {
        waitpid(pid, &status, WUNTRACED);
    }
    
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    
    if (WIFSTOPPED(status)) {
        job->stopped = true;
    } else {
        jobs.erase(std::remove_if(jobs.begin(), jobs.end(),
            [job_id](const Job& j) { return j.job_id == job_id; }), jobs.end());
    }
}

void bg_command(const std::vector<std::string>& args) {
    int job_id = -1;
    
    for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
        if (it->stopped) {
            job_id = it->job_id;
            break;
        }
    }
    
    if (!args.empty()) {
        try {
            job_id = std::stoi(args[0]);
        } catch (...) {
            std::cout << "bg: " << args[0] << ": no such job" << std::endl;
            return;
        }
    }
    
    Job* job = find_job(job_id);
    if (!job) {
        std::cout << "bg: " << job_id << ": no such job" << std::endl;
        return;
    }
    
    if (!job->stopped) {
        std::cout << "bg: job " << job_id << " already in background" << std::endl;
        return;
    }
    
    std::cout << "[" << job->job_id << "]+ " << job->command << " &" << std::endl;
    
    job->stopped = false;
    job->background = true;
    kill(-job->pgid, SIGCONT);
}

void jobs_command(const std::vector<std::string>&) {
    for (const auto& job : jobs) {
        std::cout << "[" << job.job_id << "]  ";
        if (job.stopped) {
            std::cout << "Stopped";
        } else if (job.background) {
            std::cout << "Running";
        } else {
            std::cout << "Running";
        }
        std::cout << "                 " << job.command;
        if (job.background && !job.stopped) {
            std::cout << " &";
        }
        std::cout << std::endl;
    }
}

void help_command(const std::vector<std::string>&) {
    const char* CYAN = "\033[36m";
    const char* YELLOW = "\033[33m";
    const char* RESET = "\033[0m";
    
    std::cout << YELLOW << "\nAvailable Builtin Commands:\n" << RESET;
    std::cout << std::string(50, '-') << "\n";
    std::cout << CYAN << "exit [code]" << RESET << "       - Exit the shell\n";
    std::cout << CYAN << "echo <args>" << RESET << "       - Print arguments to stdout\n";
    std::cout << CYAN << "type <cmd>" << RESET << "        - Show command type\n";
    std::cout << CYAN << "pwd" << RESET << "               - Print working directory\n";
    std::cout << CYAN << "cd [dir]" << RESET << "          - Change directory\n";
    std::cout << CYAN << "history [n]" << RESET << "       - View command history\n";
    std::cout << CYAN << "jobs" << RESET << "              - List background jobs\n";
    std::cout << CYAN << "fg [job]" << RESET << "          - Bring job to foreground\n";
    std::cout << CYAN << "bg [job]" << RESET << "          - Resume job in background\n";
    std::cout << CYAN << "help" << RESET << "              - Show this help message\n";
    std::cout << std::string(50, '-') << "\n\n";
}

void execute_builtin(const std::string& command, const std::vector<std::string>& args,
                     const RedirectionConfig& redir) {
    int saved_stdout = -1, saved_stderr = -1;
    int stdout_fd = -1, stderr_fd = -1;
    
    if (!redir.stdout_file.empty()) {
        saved_stdout = dup(STDOUT_FILENO);
        int flags = O_WRONLY | O_CREAT | (redir.stdout_append ? O_APPEND : O_TRUNC);
        stdout_fd = open(redir.stdout_file.c_str(), flags, 0644);
        if (stdout_fd != -1) {
            dup2(stdout_fd, STDOUT_FILENO);
        }
    }
    
    if (!redir.stderr_file.empty()) {
        saved_stderr = dup(STDERR_FILENO);
        int flags = O_WRONLY | O_CREAT | (redir.stderr_append ? O_APPEND : O_TRUNC);
        stderr_fd = open(redir.stderr_file.c_str(), flags, 0644);
        if (stderr_fd != -1) {
            dup2(stderr_fd, STDERR_FILENO);
        }
    }
    
    builtins[command](args);
    
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
    if (stdout_fd != -1) close(stdout_fd);
    if (stderr_fd != -1) close(stderr_fd);
}
