#ifndef BUILTINS_H
#define BUILTINS_H

#include <string>
#include <vector>
#include <map>
#include "parser.h"

typedef void (*builtin_func)(const std::vector<std::string>&);

extern std::map<std::string, builtin_func> builtins;

void init_builtins();
bool is_builtin(const std::string& cmd);
void execute_builtin(const std::string& command, const std::vector<std::string>& args,
                     const RedirectionConfig& redir);

// Individual builtin functions
void exit_command(const std::vector<std::string>& args);
void echo_command(const std::vector<std::string>& args);
void type_command(const std::vector<std::string>& args);
void pwd_command(const std::vector<std::string>& args);
void cd_command(const std::vector<std::string>& args);
void history_command(const std::vector<std::string>& args);
void fg_command(const std::vector<std::string>& args);
void bg_command(const std::vector<std::string>& args);
void jobs_command(const std::vector<std::string>& args);
void help_command(const std::vector<std::string>& args);

#endif // BUILTINS_H
