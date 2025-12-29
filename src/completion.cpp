#include "completion.h"
#include "builtins.h"
#include "utils.h"
#include <readline/readline.h>
#include <algorithm>
#include <cstring>

char* command_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t match_index;
    
    if (state == 0) {
        matches.clear();
        match_index = 0;
        
        std::string prefix(text);
        
        for (const auto& builtin : builtins) {
            if (builtin.first.find(prefix) == 0) {
                matches.push_back(builtin.first);
            }
        }
        
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
