#include "executor.h"
#include "parser.h"
#include "builtins.h"
#include "job_control.h"
#include "shell.h"
#include "utils.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>

extern pid_t shell_pgid;
extern bool shell_is_interactive;

void execute_external(const std::string& command, const std::vector<std::string>& args, 
                      const RedirectionConfig& redir, int input_fd, int output_fd,
                      bool in_background, pid_t pgid) {
    std::string executable_path = find_executable_in_path(command);
    
    if (executable_path.empty()) {
        std::cout << command << ": command not found" << std::endl;
        return;
    }
    
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
            signal(SIGCHLD, SIG_DFL);
        }
        
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
        
        if (!redir.stderr_file.empty()) {
            int flags = O_WRONLY | O_CREAT | (redir.stderr_append ? O_APPEND : O_TRUNC);
            int fd = open(redir.stderr_file.c_str(), flags, 0644);
            if (fd != -1) {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        
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

void execute_ast_node(ASTNode* node, bool in_background) {
    if (!node) return;
    
    switch (node->type) {
        case NodeType::COMMAND: {
            if (is_builtin(node->command)) {
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
                
                if (!is_builtin(cmd_node->command)) {
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
                } else {
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
                        
                        execute_builtin(cmd_node->command, cmd_node->args, cmd_node->redir);
                        std::exit(0);
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
                add_job(pgid, "", pids, true);
                std::cout << "[" << (next_job_id - 1) << "] " << pgid << std::endl;
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
