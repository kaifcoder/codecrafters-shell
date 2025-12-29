#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <termios.h>
#include <unistd.h>

extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern bool shell_is_interactive;

void init_shell();
std::string get_prompt();
void print_welcome_message();
void setup_readline(std::string& history_file);
void save_history(const std::string& history_file);
void run_shell();

#endif // SHELL_H
