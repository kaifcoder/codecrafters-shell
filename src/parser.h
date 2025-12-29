#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>

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

std::string expand_command_substitution(const std::string& input);
std::vector<std::string> parse_arguments(const std::string& input);
std::pair<std::vector<std::string>, RedirectionConfig> parse_redirection(const std::vector<std::string>& parts);
std::vector<std::string> parse_pipeline(const std::string& input);
std::unique_ptr<ASTNode> parse_to_ast(const std::string& input);

#endif // PARSER_H
