#include "utils.h"
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <cstdlib>

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
