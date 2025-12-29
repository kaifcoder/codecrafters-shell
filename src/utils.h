#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string trim(const std::string& str);
std::string find_executable_in_path(const std::string& cmd);
std::vector<std::string> get_all_executables();

#endif // UTILS_H
