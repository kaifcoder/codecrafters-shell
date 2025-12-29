#include "parser.h"
#include "executor.h"
#include "heredoc.h"
#include "utils.h"
#include <unistd.h>
#include <sys/wait.h>

std::string execute_for_output(const std::string& cmd);

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
        
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    return "";
}

std::vector<std::string> parse_arguments(const std::string& input) {
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

std::pair<std::vector<std::string>, RedirectionConfig> parse_redirection(const std::vector<std::string>& parts) {
    std::vector<std::string> filtered;
    RedirectionConfig redir;
    
    for (size_t i = 0; i < parts.size(); i++) {
        std::string token = parts[i];
        std::string next_token = (i + 1 < parts.size()) ? parts[i + 1] : "";
        
        if (token == "<<") {
            redir.use_heredoc = true;
            redir.heredoc_delimiter = next_token;
            i++;
            continue;
        }
        
        if (token == "<") {
            redir.stdin_file = next_token;
            i++;
            continue;
        }
        
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

std::unique_ptr<ASTNode> parse_to_ast(const std::string& input) {
    std::string cmd = trim(input);
    bool is_background = false;
    
    if (!cmd.empty() && cmd.back() == '&') {
        is_background = true;
        cmd = trim(cmd.substr(0, cmd.length() - 1));
    }
    
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
