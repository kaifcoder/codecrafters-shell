#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "utils.h"
#include "job_control.h"
#include <iostream>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <cstring>

pid_t shell_pgid;
struct termios shell_tmodes;
bool shell_is_interactive;

// Signal handlers
void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        for (auto& job : jobs) {
            auto it = std::find(job.pids.begin(), job.pids.end(), pid);
            if (it != job.pids.end()) {
                if (WIFSTOPPED(status)) {
                    job.stopped = true;
                    if (!job.background) {
                        std::cerr << "\n[" << job.job_id << "]+ Stopped   " 
                                  << job.command << std::endl;
                    }
                } else if (WIFCONTINUED(status)) {
                    job.stopped = false;
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    job.pids.erase(it);
                    if (job.pids.empty() && job.background) {
                        std::cerr << "\n[" << job.job_id << "]+ Done       " 
                                  << job.command << std::endl;
                    }
                }
            }
        }
    }
    
    // Remove completed jobs
    jobs.erase(std::remove_if(jobs.begin(), jobs.end(),
        [](const Job& j) { return j.pids.empty(); }), jobs.end());
}

void sigint_handler(int sig) {
    (void)sig;
    std::cout << std::endl;
    rl_on_new_line();
    rl_redisplay();
}

void sigtstp_handler(int sig) {
    (void)sig;
}

void init_shell() {
    shell_is_interactive = isatty(STDIN_FILENO);
    
    if (shell_is_interactive) {
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("setpgid");
            exit(1);
        }
        
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        tcgetattr(STDIN_FILENO, &shell_tmodes);
        
        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, sigtstp_handler);
        signal(SIGCHLD, sigchld_handler);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
    }
}

std::string get_prompt() {
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        return "$ ";
    }
    
    std::string path = cwd;
    const char* home = std::getenv("HOME");
    if (home && path.find(home) == 0) {
        path = "~" + path.substr(strlen(home));
    }
    
    const char* user = std::getenv("USER");
    if (!user) user = std::getenv("LOGNAME");
    
    const char* GREEN = "\033[32m";
    const char* BLUE = "\033[34m";
    const char* RESET = "\033[0m";
    
    if (user) {
        return std::string(GREEN) + user + RESET + ":" + BLUE + path + RESET + "$ ";
    } else {
        return BLUE + path + RESET + "$ ";
    }
}

void print_welcome_message() {
    const char* CYAN = "\033[36m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    
    std::cout << CYAN << BOLD << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║                                                ║\n";
    std::cout << "║         " << RESET << CYAN << "Welcome to Custom C++ Shell" << BOLD << "         ║\n";
    std::cout << "║                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════╝" << RESET << "\n\n";
    
    const char* user = std::getenv("USER");
    if (user) {
        std::cout << GREEN << "👤 User: " << RESET << user << std::endl;
    }
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        std::cout << GREEN << "💻 Host: " << RESET << hostname << std::endl;
    }
    
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) {
        std::cout << GREEN << "📁 Working Directory: " << RESET << cwd << std::endl;
    }
    
    std::cout << "\n" << YELLOW << "Features:" << RESET << "\n";
    std::cout << "  • Job Control (bg, fg, jobs)\n";
    std::cout << "  • Command Substitution $(...)  \n";
    std::cout << "  • Pipelines & Redirects\n";
    std::cout << "  • Tab Completion\n";
    std::cout << "  • Command History (↑/↓)\n";
    std::cout << "  • Signal Handling (Ctrl+C, Ctrl+Z)\n";
    
    std::cout << "\n" << YELLOW << "Quick Tips:" << RESET << "\n";
    std::cout << "  • Use " << CYAN << "Tab" << RESET << " for command completion\n";
    std::cout << "  • Use " << CYAN << "Ctrl+C" << RESET << " to stop current command\n";
    std::cout << "  • Use " << CYAN << "Ctrl+Z" << RESET << " to suspend current job\n";
    std::cout << "  • Use " << CYAN << "Ctrl+D" << RESET << " or type " << CYAN << "exit" << RESET << " to quit\n";
    std::cout << "  • Type " << CYAN << "help" << RESET << " for available builtins\n";
    
    std::cout << "\n" << std::string(50, '-') << "\n\n";
}

void setup_readline(std::string& history_file) {
    extern char** command_completion(const char*, int, int);
    rl_attempted_completion_function = command_completion;
    
    const char* histfile_env = std::getenv("HISTFILE");
    if (histfile_env) {
        history_file = histfile_env;
    } else {
        const char* home = std::getenv("HOME");
        history_file = std::string(home ? home : ".") + "/.shell_history";
    }
    
    stifle_history(500);
    read_history(history_file.c_str());
}

void save_history(const std::string& history_file) {
    write_history(history_file.c_str());
}

void run_shell() {
    std::string history_file;
    setup_readline(history_file);
    print_welcome_message();
    
    while (true) {
        std::string prompt = get_prompt();
        char* line = readline(prompt.c_str());
        
        if (!line) {
            std::cout << std::endl;
            break;
        }
        
        std::string input = trim(line);
        
        if (!input.empty()) {
            add_history(input.c_str());
            process_command(input);
        }
        
        free(line);
    }
    
    save_history(history_file);
}
