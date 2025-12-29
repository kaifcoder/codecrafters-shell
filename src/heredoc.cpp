#include "heredoc.h"
#include <readline/readline.h>
#include <string>

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
