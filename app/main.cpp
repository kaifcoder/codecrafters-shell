#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <dirent.h>
#include <signal.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>

// Global tracking for history positions
std::map<std::string, int> last_written_positions;

// Job control structures
struct Job {
    int job_id;
    pid_t pgid;
    std::string command;
    bool stopped;
    bool background;
    std::vector<pid_t> pids;
};

std::vector<Job> jobs;
int next_job_id = 1;
pid_t shell_pgid;
struct termios shell_tmodes;
bool shell_is_interactive;

struct RedirectionConfig {
    std::string stdout_file;
    std::string stderr_file;
    std::string stdin_file;
    std::string heredoc_delimiter;
    std::string heredoc_content;
    bool stdout_append = false;
    bool stderr_append = false;
    bool use_heredoc = false;
    int stdin_pipe = -1;
};

// AST Node types
enum class NodeType {
    COMMAND,
    PIPELINE,
    BACKGROUND,
    SEQUENCE
};

struct ASTNode {
    NodeType type;
    std::string command;
    std::vector<std::string> args;
    RedirectionConfig redir;
    std::vector<std::unique_ptr<ASTNode>> children;
    
    ASTNode(NodeType t) : type(t) {}
};

// Forward declarations
void process_command(const std::string& input);
std::vector<std::string> get_all_executables();
void execute_ast_node(ASTNode* node, bool in_background = false);
std::string expand_command_substitution(const std::string& input);

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
    // Don't exit shell, just print newline
    std::cout << std::endl;
    rl_on_new_line();
    rl_redisplay();
}

void sigtstp_handler(int sig) {
    (void)sig;
    // Ignore SIGTSTP in shell
}

void init_shell() {
    shell_is_interactive = isatty(STDIN_FILENO);
    
    if (shell_is_interactive) {
        // Put shell in its own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("setpgid");
            exit(1);
        }
        
        // Take control of terminal
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        
        // Save terminal attributes
        tcgetattr(STDIN_FILENO, &shell_tmodes);
        
        // Setup signal handlers
        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, sigtstp_handler);
        signal(SIGCHLD, sigchld_handler);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
    }
}

// Utility functions
std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

// Command substitution helper
std::string execute_for_output(const std::string& cmd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return "";
    
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        process_command(cmd);
        exit(0);
    } else if (pid > 0) {
        close(pipefd[1]);
        std::string output;
        char buffer[4096];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, n);
        }
        close(pipefd[0]);
        waitpid(pid, nullptr, 0);
        
        // Remove trailing newline
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    return "";
}

std::string expand_command_substitution(const std::string& input) {
    std::string result;
    bool in_single_quote = false;
    bool in_double_quote = false;
    
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            result += input[i];
        } else if (input[i] == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            result += input[i];
        } else if (input[i] == '$' && i + 1 < input.length() && input[i + 1] == '(' && !in_single_quote) {
            // Find matching parenthesis
            size_t start = i + 2;
            int depth = 1;
            size_t end = start;
            
            while (end < input.length() && depth > 0) {
                if (input[end] == '(') depth++;
                else if (input[end] == ')') depth--;
                if (depth > 0) end++;
            }
            
            if (depth == 0) {
                std::string cmd = input.substr(start, end - start);
                std::string output = execute_for_output(cmd);
                result += output;
                i = end;
            } else {
                result += input[i];
            }
        } else {
            result += input[i];
        }
    }
    
    return result;
}

std::vector<std::string> parse_arguments(const std::string& input) {
    // First expand command substitutions
    std::string expanded = expand_command_substitution(input);
    
    std::vector<std::string> args;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;
    
    for (size_t i = 0; i < expanded.length(); i++) {
        char c = expanded[i];
        
        if (escaped) {
            current += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\' && !in_single_quote) {
            escaped = true;
            continue;
        }
        
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            continue;
        }
        
        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            continue;
        }
        
        if ((c == ' ' || c == '\t') && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        args.push_back(current);
    }
    
    return args;
}

std::string find_executable_in_path(const std::string& cmd) {
    const char* path_env = std::getenv("PATH");
    if (!path_env) return "";
    
    std::vector<std::string> directories = split_string(path_env, ':');
    
    for (const auto& dir : directories) {
        std::string file_path = dir + "/" + cmd;
        struct stat sb;
        if (stat(file_path.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR)) {
            return file_path;
        }
    }
    
    return "";
}

// Builtin commands
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

void type_command(const std::vector<std::string>& args);

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
    
    auto it = std::find_if(jobs.begin(), jobs.end(),
        [job_id](const Job& j) { return j.job_id == job_id; });
    
    if (it == jobs.end()) {
        std::cout << "fg: " << job_id << ": no such job" << std::endl;
        return;
    }
    
    std::cout << it->command << std::endl;
    
    // Continue if stopped
    if (it->stopped) {
        kill(-it->pgid, SIGCONT);
        it->stopped = false;
    }
    
    // Give terminal to job
    tcsetpgrp(STDIN_FILENO, it->pgid);
    it->background = false;
    
    // Wait for job
    int status;
    for (pid_t pid : it->pids) {
        waitpid(pid, &status, WUNTRACED);
    }
    
    // Take back terminal
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    
    if (WIFSTOPPED(status)) {
        it->stopped = true;
    } else {
        jobs.erase(it);
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
    
    auto it = std::find_if(jobs.begin(), jobs.end(),
        [job_id](const Job& j) { return j.job_id == job_id; });
    
    if (it == jobs.end()) {
        std::cout << "bg: " << job_id << ": no such job" << std::endl;
        return;
    }
    
    if (!it->stopped) {
        std::cout << "bg: job " << job_id << " already in background" << std::endl;
        return;
    }
    
    std::cout << "[" << it->job_id << "]+ " << it->command << " &" << std::endl;
    
    it->stopped = false;
    it->background = true;
    kill(-it->pgid, SIGCONT);
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

void history_command(const std::vector<std::string>& args) {
    // Check for -r flag (read from file)
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
    
    // Check for -w flag (write to file)
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
    
    // Check for -a flag (append to file)
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
    
    // Display history
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

// Map of builtin commands
std::map<std::string, void(*)(const std::vector<std::string>&)> builtins = {
    {"exit", exit_command},
    {"echo", echo_command},
    {"type", type_command},
    {"pwd", pwd_command},
    {"cd", cd_command},
    {"history", history_command},
    {"fg", fg_command},
    {"bg", bg_command},
    {"jobs", jobs_command},
    {"help", help_command}
};

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

std::vector<std::string> get_all_executables() {
    std::vector<std::string> executables;
    const char* path_env = std::getenv("PATH");
    if (!path_env) return executables;
    
    std::vector<std::string> directories = split_string(path_env, ':');
    
    for (const auto& dir : directories) {
        DIR* dirp = opendir(dir.c_str());
        if (!dirp) continue;
        
        struct dirent* entry;
        while ((entry = readdir(dirp)) != nullptr) {
            std::string file_path = dir + "/" + entry->d_name;
            struct stat sb;
            if (stat(file_path.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR)) {
                executables.push_back(entry->d_name);
            }
        }
        closedir(dirp);
    }
    
    // Remove duplicates
    std::sort(executables.begin(), executables.end());
    executables.erase(std::unique(executables.begin(), executables.end()), executables.end());
    
    return executables;
}

// Tab completion
char* command_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t match_index;
    
    if (state == 0) {
        matches.clear();
        match_index = 0;
        
        std::string prefix(text);
        
        // Add builtins
        for (const auto& builtin : builtins) {
            if (builtin.first.find(prefix) == 0) {
                matches.push_back(builtin.first);
            }
        }
        
        // Add executables
        auto executables = get_all_executables();
        for (const auto& exe : executables) {
            if (exe.find(prefix) == 0) {
                matches.push_back(exe);
            }
        }
        
        std::sort(matches.begin(), matches.end());
    }
    
    if (match_index < matches.size()) {
        return strdup(matches[match_index++].c_str());
    }
    
    return nullptr;
}

char** command_completion(const char* text, int start, int) {
    rl_attempted_completion_over = 1;
    
    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }
    
    return nullptr;
}

void setup_readline(std::string& history_file) {
    // Setup tab completion
    rl_attempted_completion_function = command_completion;
    
    // Get history file path
    const char* histfile_env = std::getenv("HISTFILE");
    if (histfile_env) {
        history_file = histfile_env;
    } else {
        const char* home = std::getenv("HOME");
        history_file = std::string(home ? home : ".") + "/.shell_history";
    }
    
    // Load history
    stifle_history(500);
    read_history(history_file.c_str());
}

void save_history(const std::string& history_file) {
    write_history(history_file.c_str());
}

std::string get_prompt() {
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        return "$ ";
    }
    
    std::string path = cwd;
    
    // Replace home directory with ~
    const char* home = std::getenv("HOME");
    if (home && path.find(home) == 0) {
        path = "~" + path.substr(strlen(home));
    }
    
    // Get username
    const char* user = std::getenv("USER");
    if (!user) user = std::getenv("LOGNAME");
    
    // ANSI color codes
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
    // ANSI color codes
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
    
    // User info
    const char* user = std::getenv("USER");
    if (user) {
        std::cout << GREEN << "👤 User: " << RESET << user << std::endl;
    }
    
    // System info
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

// Redirection parsing with heredoc support
std::pair<std::vector<std::string>, RedirectionConfig> parse_redirection(const std::vector<std::string>& parts) {
    std::vector<std::string> filtered;
    RedirectionConfig redir;
    
    for (size_t i = 0; i < parts.size(); i++) {
        std::string token = parts[i];
        std::string next_token = (i + 1 < parts.size()) ? parts[i + 1] : "";
        
        // Check for heredoc
        if (token == "<<") {
            redir.use_heredoc = true;
            redir.heredoc_delimiter = next_token;
            i++;
            continue;
        }
        
        // Check for input redirection
        if (token == "<") {
            redir.stdin_file = next_token;
            i++;
            continue;
        }
        
        // Check for redirection operators
        if (token == ">" || token == "1>") {
            redir.stdout_file = next_token;
            redir.stdout_append = false;
            i++;
        } else if (token == ">>" || token == "1>>") {
            redir.stdout_file = next_token;
            redir.stdout_append = true;
            i++;
        } else if (token == "2>") {
            redir.stderr_file = next_token;
            redir.stderr_append = false;
            i++;
        } else if (token == "2>>") {
            redir.stderr_file = next_token;
            redir.stderr_append = true;
            i++;
        } else if (token.substr(0, 2) == "<<") {
            if (token.length() > 2) {
                redir.use_heredoc = true;
                redir.heredoc_delimiter = token.substr(2);
            }
        } else if (token.substr(0, 2) == "2>>" || token.substr(0, 3) == "1>>") {
            // Inline redirection like 2>>file
            size_t op_len = (token[0] == '2') ? 3 : 3;
            if (token.length() > op_len) {
                std::string filename = token.substr(op_len);
                if (token[0] == '2') {
                    redir.stderr_file = filename;
                    redir.stderr_append = true;
                } else {
                    redir.stdout_file = filename;
                    redir.stdout_append = true;
                }
            }
        } else if (token.substr(0, 2) == "2>" || token.substr(0, 2) == "1>" || token.substr(0, 2) == ">>") {
            size_t op_len = 2;
            if (token.length() > op_len) {
                std::string filename = token.substr(op_len);
                if (token[0] == '2') {
                    redir.stderr_file = filename;
                    redir.stderr_append = false;
                } else if (token[0] == '1') {
                    redir.stdout_file = filename;
                    redir.stdout_append = false;
                } else {
                    redir.stdout_file = filename;
                    redir.stdout_append = true;
                }
            }
        } else if (token.substr(0, 1) == ">") {
            if (token.length() > 1) {
                std::string filename = token.substr(1);
                redir.stdout_file = filename;
                redir.stdout_append = false;
            }
        } else {
            filtered.push_back(token);
        }
    }
    
    return {filtered, redir};
}

// Read heredoc content interactively
void read_heredoc(RedirectionConfig& redir) {
    if (!redir.use_heredoc || redir.heredoc_delimiter.empty()) return;
    
    std::string content;
    std::string line;
    
    while (true) {
        char* input = readline("> ");
        if (!input) break;
        
        line = input;
        free(input);
        
        if (line == redir.heredoc_delimiter) {
            break;
        }
        
        content += line + "\n";
    }
    
    redir.heredoc_content = content;
}

// Pipeline parsing
std::vector<std::string> parse_pipeline(const std::string& input) {
    std::vector<std::string> commands;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;
    
    for (char c : input) {
        if (escaped) {
            current += c;
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            current += c;
            continue;
        }
        
        if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            current += c;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            current += c;
        } else if (c == '|' && !in_single_quote && !in_double_quote) {
            commands.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        commands.push_back(trim(current));
    }
    
    return commands;
}

void execute_external(const std::string& command, const std::vector<std::string>& args, 
                      const RedirectionConfig& redir, int input_fd = -1, int output_fd = -1,
                      bool in_background = false, pid_t pgid = 0) {
    std::string executable_path = find_executable_in_path(command);
    
    if (executable_path.empty()) {
        std::cout << command << ": command not found" << std::endl;
        return;
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        
        // Set process group for job control
        if (shell_is_interactive) {
            pid = getpid();
            if (pgid == 0) pgid = pid;
            setpgid(pid, pgid);
            
            if (!in_background) {
                tcsetpgrp(STDIN_FILENO, pgid);
            }
            
            // Reset signals
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
        }
        
        // Handle heredoc
        if (redir.use_heredoc && !redir.heredoc_content.empty()) {
            int pipefd[2];
            if (pipe(pipefd) == 0) {
                write(pipefd[1], redir.heredoc_content.c_str(), redir.heredoc_content.length());
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
            }
        } else if (!redir.stdin_file.empty()) {
            int fd = open(redir.stdin_file.c_str(), O_RDONLY);
            if (fd != -1) {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        } else if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        // Handle output redirection
        if (!redir.stdout_file.empty()) {
            int flags = O_WRONLY | O_CREAT | (redir.stdout_append ? O_APPEND : O_TRUNC);
            int fd = open(redir.stdout_file.c_str(), flags, 0644);
            if (fd != -1) {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        } else if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Handle stderr redirection
        if (!redir.stderr_file.empty()) {
            int flags = O_WRONLY | O_CREAT | (redir.stderr_append ? O_APPEND : O_TRUNC);
            int fd = open(redir.stderr_file.c_str(), flags, 0644);
            if (fd != -1) {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        
        // Prepare arguments
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execv(executable_path.c_str(), argv.data());
        std::cerr << command << ": exec failed" << std::endl;
        std::exit(1);
    } else if (pid > 0) {
        // Parent process
        if (shell_is_interactive) {
            if (pgid == 0) pgid = pid;
            setpgid(pid, pgid);
        }
        
        if (!in_background) {
            int status;
            waitpid(pid, &status, WUNTRACED);
            
            if (shell_is_interactive) {
                tcsetpgrp(STDIN_FILENO, shell_pgid);
            }
        }
    } else {
        std::cerr << "fork failed" << std::endl;
    }
}

void execute_builtin(const std::string& command, const std::vector<std::string>& args,
                     const RedirectionConfig& redir) {
    int saved_stdout = -1, saved_stderr = -1;
    int stdout_fd = -1, stderr_fd = -1;
    
    // Handle stdout redirection
    if (!redir.stdout_file.empty()) {
        saved_stdout = dup(STDOUT_FILENO);
        int flags = O_WRONLY | O_CREAT | (redir.stdout_append ? O_APPEND : O_TRUNC);
        stdout_fd = open(redir.stdout_file.c_str(), flags, 0644);
        if (stdout_fd != -1) {
            dup2(stdout_fd, STDOUT_FILENO);
        }
    }
    
    // Handle stderr redirection
    if (!redir.stderr_file.empty()) {
        saved_stderr = dup(STDERR_FILENO);
        int flags = O_WRONLY | O_CREAT | (redir.stderr_append ? O_APPEND : O_TRUNC);
        stderr_fd = open(redir.stderr_file.c_str(), flags, 0644);
        if (stderr_fd != -1) {
            dup2(stderr_fd, STDERR_FILENO);
        }
    }
    
    // Execute builtin
    builtins[command](args);
    
    // Restore stdout/stderr
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

void execute_pipeline(const std::vector<std::string>& commands) {
    if (commands.size() == 1) {
        process_command(commands[0]);
        return;
    }
    
    std::vector<std::tuple<std::string, std::vector<std::string>, RedirectionConfig>> parsed_commands;
    
    for (const auto& cmd_str : commands) {
        auto parts = parse_arguments(cmd_str);
        auto [filtered, redir] = parse_redirection(parts);
        
        if (!filtered.empty()) {
            std::string cmd = filtered[0];
            std::vector<std::string> args(filtered.begin() + 1, filtered.end());
            parsed_commands.push_back({cmd, args, redir});
        }
    }
    
    if (parsed_commands.empty()) return;
    
    std::vector<int> pipe_fds;
    
    for (size_t i = 0; i < parsed_commands.size(); i++) {
        auto& [cmd, args, redir] = parsed_commands[i];
        
        int input_fd = -1;
        int output_fd = -1;
        
        // Get input from previous pipe
        if (i > 0) {
            input_fd = pipe_fds[(i - 1) * 2];
        }
        
        // Create pipe for next command
        if (i < parsed_commands.size() - 1) {
            int pipefd[2];
            if (pipe(pipefd) == 0) {
                pipe_fds.push_back(pipefd[0]);
                pipe_fds.push_back(pipefd[1]);
                output_fd = pipefd[1];
            }
        }
        
        // Fork for external commands
        if (builtins.find(cmd) == builtins.end()) {
            pid_t pid = fork();
            
            if (pid == 0) {
                // Child process
                if (input_fd != -1) {
                    dup2(input_fd, STDIN_FILENO);
                }
                if (output_fd != -1) {
                    dup2(output_fd, STDOUT_FILENO);
                }
                
                // Close all pipe fds
                for (int fd : pipe_fds) {
                    close(fd);
                }
                
                // Handle file redirections
                if (!redir.stdout_file.empty()) {
                    int flags = O_WRONLY | O_CREAT | (redir.stdout_append ? O_APPEND : O_TRUNC);
                    int fd = open(redir.stdout_file.c_str(), flags, 0644);
                    if (fd != -1) {
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                }
                
                if (!redir.stderr_file.empty()) {
                    int flags = O_WRONLY | O_CREAT | (redir.stderr_append ? O_APPEND : O_TRUNC);
                    int fd = open(redir.stderr_file.c_str(), flags, 0644);
                    if (fd != -1) {
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                    }
                }
                
                std::string executable_path = find_executable_in_path(cmd);
                if (executable_path.empty()) {
                    std::cerr << cmd << ": command not found" << std::endl;
                    std::exit(127);
                }
                
                std::vector<char*> argv;
                argv.push_back(const_cast<char*>(cmd.c_str()));
                for (const auto& arg : args) {
                    argv.push_back(const_cast<char*>(arg.c_str()));
                }
                argv.push_back(nullptr);
                
                execv(executable_path.c_str(), argv.data());
                std::exit(127);
            }
        } else {
            // Builtin command in pipeline - handle with fork to support piping
            pid_t pid = fork();
            
            if (pid == 0) {
                if (input_fd != -1) {
                    dup2(input_fd, STDIN_FILENO);
                }
                if (output_fd != -1) {
                    dup2(output_fd, STDOUT_FILENO);
                }
                
                for (int fd : pipe_fds) {
                    close(fd);
                }
                
                execute_builtin(cmd, args, redir);
                std::exit(0);
            }
        }
        
        // Parent closes pipe ends
        if (input_fd != -1) {
            close(input_fd);
        }
        if (output_fd != -1) {
            close(output_fd);
        }
    }
    
    // Close all remaining pipe fds and wait for children
    for (int fd : pipe_fds) {
        close(fd);
    }
    
    for (size_t i = 0; i < parsed_commands.size(); i++) {
        wait(nullptr);
    }
}

// AST-based parser
std::unique_ptr<ASTNode> parse_to_ast(const std::string& input) {
    // Check for background operator at end
    std::string cmd = trim(input);
    bool is_background = false;
    
    if (!cmd.empty() && cmd.back() == '&') {
        is_background = true;
        cmd = trim(cmd.substr(0, cmd.length() - 1));
    }
    
    // Parse pipeline
    auto pipeline_parts = parse_pipeline(cmd);
    
    if (pipeline_parts.size() > 1) {
        auto pipeline_node = std::make_unique<ASTNode>(NodeType::PIPELINE);
        
        for (const auto& part : pipeline_parts) {
            auto parts = parse_arguments(part);
            auto [filtered, redir] = parse_redirection(parts);
            
            if (redir.use_heredoc) {
                read_heredoc(redir);
            }
            
            if (!filtered.empty()) {
                auto cmd_node = std::make_unique<ASTNode>(NodeType::COMMAND);
                cmd_node->command = filtered[0];
                cmd_node->args = std::vector<std::string>(filtered.begin() + 1, filtered.end());
                cmd_node->redir = redir;
                pipeline_node->children.push_back(std::move(cmd_node));
            }
        }
        
        if (is_background) {
            auto bg_node = std::make_unique<ASTNode>(NodeType::BACKGROUND);
            bg_node->children.push_back(std::move(pipeline_node));
            return bg_node;
        }
        
        return pipeline_node;
    } else {
        // Single command
        auto parts = parse_arguments(cmd);
        auto [filtered, redir] = parse_redirection(parts);
        
        if (redir.use_heredoc) {
            read_heredoc(redir);
        }
        
        if (!filtered.empty()) {
            auto cmd_node = std::make_unique<ASTNode>(NodeType::COMMAND);
            cmd_node->command = filtered[0];
            cmd_node->args = std::vector<std::string>(filtered.begin() + 1, filtered.end());
            cmd_node->redir = redir;
            
            if (is_background) {
                auto bg_node = std::make_unique<ASTNode>(NodeType::BACKGROUND);
                bg_node->children.push_back(std::move(cmd_node));
                return bg_node;
            }
            
            return cmd_node;
        }
    }
    
    return nullptr;
}

void execute_ast_node(ASTNode* node, bool in_background) {
    if (!node) return;
    
    switch (node->type) {
        case NodeType::COMMAND: {
            if (builtins.find(node->command) != builtins.end()) {
                execute_builtin(node->command, node->args, node->redir);
            } else {
                execute_external(node->command, node->args, node->redir, -1, -1, in_background);
            }
            break;
        }
        
        case NodeType::PIPELINE: {
            if (node->children.size() == 1) {
                execute_ast_node(node->children[0].get(), in_background);
                return;
            }
            
            std::vector<int> pipe_fds;
            std::vector<pid_t> pids;
            pid_t pgid = 0;
            
            for (size_t i = 0; i < node->children.size(); i++) {
                auto* cmd_node = node->children[i].get();
                
                int input_fd = -1;
                int output_fd = -1;
                
                if (i > 0) {
                    input_fd = pipe_fds[(i - 1) * 2];
                }
                
                if (i < node->children.size() - 1) {
                    int pipefd[2];
                    if (pipe(pipefd) == 0) {
                        pipe_fds.push_back(pipefd[0]);
                        pipe_fds.push_back(pipefd[1]);
                        output_fd = pipefd[1];
                    }
                }
                
                if (builtins.find(cmd_node->command) == builtins.end()) {
                    pid_t pid = fork();
                    
                    if (pid == 0) {
                        if (shell_is_interactive) {
                            pid = getpid();
                            if (pgid == 0) pgid = pid;
                            setpgid(pid, pgid);
                            
                            if (!in_background) {
                                tcsetpgrp(STDIN_FILENO, pgid);
                            }
                            
                            signal(SIGINT, SIG_DFL);
                            signal(SIGQUIT, SIG_DFL);
                            signal(SIGTSTP, SIG_DFL);
                            signal(SIGTTIN, SIG_DFL);
                            signal(SIGTTOU, SIG_DFL);
                        }
                        
                        if (input_fd != -1) {
                            dup2(input_fd, STDIN_FILENO);
                        }
                        if (output_fd != -1) {
                            dup2(output_fd, STDOUT_FILENO);
                        }
                        
                        for (int fd : pipe_fds) {
                            close(fd);
                        }
                        
                        if (!cmd_node->redir.stdout_file.empty()) {
                            int flags = O_WRONLY | O_CREAT | (cmd_node->redir.stdout_append ? O_APPEND : O_TRUNC);
                            int fd = open(cmd_node->redir.stdout_file.c_str(), flags, 0644);
                            if (fd != -1) {
                                dup2(fd, STDOUT_FILENO);
                                close(fd);
                            }
                        }
                        
                        std::string executable_path = find_executable_in_path(cmd_node->command);
                        if (executable_path.empty()) {
                            std::cerr << cmd_node->command << ": command not found" << std::endl;
                            std::exit(127);
                        }
                        
                        std::vector<char*> argv;
                        argv.push_back(const_cast<char*>(cmd_node->command.c_str()));
                        for (const auto& arg : cmd_node->args) {
                            argv.push_back(const_cast<char*>(arg.c_str()));
                        }
                        argv.push_back(nullptr);
                        
                        execv(executable_path.c_str(), argv.data());
                        std::exit(127);
                    } else if (pid > 0) {
                        if (shell_is_interactive) {
                            if (pgid == 0) pgid = pid;
                            setpgid(pid, pgid);
                        }
                        pids.push_back(pid);
                    }
                }
                
                if (input_fd != -1) close(input_fd);
                if (output_fd != -1) close(output_fd);
            }
            
            for (int fd : pipe_fds) {
                close(fd);
            }
            
            if (!in_background && !pids.empty()) {
                for (pid_t pid : pids) {
                    waitpid(pid, nullptr, 0);
                }
                
                if (shell_is_interactive) {
                    tcsetpgrp(STDIN_FILENO, shell_pgid);
                }
            } else if (in_background && !pids.empty()) {
                Job job;
                job.job_id = next_job_id++;
                job.pgid = pgid;
                job.command = "";  // Would need to reconstruct from AST
                job.stopped = false;
                job.background = true;
                job.pids = pids;
                jobs.push_back(job);
                
                std::cout << "[" << job.job_id << "] " << pgid << std::endl;
            }
            
            break;
        }
        
        case NodeType::BACKGROUND: {
            if (!node->children.empty()) {
                execute_ast_node(node->children[0].get(), true);
            }
            break;
        }
        
        case NodeType::SEQUENCE: {
            for (auto& child : node->children) {
                execute_ast_node(child.get(), in_background);
            }
            break;
        }
    }
}

void process_command(const std::string& input) {
    auto ast = parse_to_ast(input);
    execute_ast_node(ast.get(), false);
}

int main() {
    init_shell();
    
    std::string history_file;
    setup_readline(history_file);
    
    // Display welcome message
    print_welcome_message();
    
    while (true) {
        std::string prompt = get_prompt();
        char* line = readline(prompt.c_str());
        
        if (!line) {
            // EOF (Ctrl+D)
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
    
    return 0;
}
