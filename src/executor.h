#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"
#include <string>
#include <vector>

void process_command(const std::string& input);
void execute_ast_node(ASTNode* node, bool in_background = false);
void execute_external(const std::string& command, const std::vector<std::string>& args, 
                      const RedirectionConfig& redir, int input_fd = -1, int output_fd = -1,
                      bool in_background = false, pid_t pgid = 0);

#endif // EXECUTOR_H
