# C++ Shell - Modular Architecture

## Project Structure

```
src/
├── main.cpp          - Entry point, initializes shell and builtins
├── shell.cpp/.h      - Shell initialization, main loop, signal handlers, prompt
├── parser.cpp/.h     - Lexer, AST building, command substitution, argument parsing
├── executor.cpp/.h   - Command execution (fork/exec), AST traversal, pipelines
├── job_control.cpp/.h- Job management (background jobs, fg/bg commands)
├── builtins.cpp/.h   - All builtin commands (exit, echo, cd, pwd, etc.)
├── completion.cpp/.h - Tab completion for commands
├── heredoc.cpp/.h    - Heredoc (<<) input handling
└── utils.cpp/.h      - Utility functions (trim, split, find_executable)
```

## Module Responsibilities

### main.cpp
- Entry point for the shell
- Calls `init_shell()` to set up terminal and signals
- Calls `init_builtins()` to register builtin commands
- Calls `run_shell()` to start the main loop

### shell.cpp/shell.h
- **Signal Handlers**: `sigchld_handler`, `sigint_handler`, `sigtstp_handler`
- **Shell Initialization**: `init_shell()` - sets up process groups, terminal control
- **Prompt Generation**: `get_prompt()` - creates colored prompt with user@path
- **Welcome Message**: `print_welcome_message()` - displays startup banner
- **Readline Setup**: `setup_readline()`, `save_history()` - history management
- **Main Loop**: `run_shell()` - reads input and dispatches commands

### parser.cpp/parser.h
- **Command Substitution**: `expand_command_substitution()` - handles `$(...)` syntax
- **Argument Parsing**: `parse_arguments()` - tokenizes input with quote handling
- **Redirection Parsing**: `parse_redirection()` - extracts `>`, `>>`, `2>`, `<` operators
- **Pipeline Parsing**: `parse_pipeline()` - splits commands on `|`
- **AST Builder**: `parse_to_ast()` - converts input string to AST structure
- **AST Node Types**: COMMAND, PIPELINE, BACKGROUND, SEQUENCE

### executor.cpp/executor.h
- **Command Dispatcher**: `process_command()` - main entry point for command execution
- **AST Execution**: `execute_ast_node()` - recursive AST traversal and execution
- **External Commands**: `execute_external()` - fork/exec with I/O redirection
- **Pipeline Execution**: Handles multi-command pipelines with proper piping
- **Background Jobs**: Manages background process execution (`&` operator)

### job_control.cpp/job_control.h
- **Job Structure**: `Job` struct with job_id, pgid, command, status, pids
- **Job Storage**: Global `jobs` vector and `next_job_id` counter
- **Job Management**: `add_job()`, `find_job()`, `remove_completed_jobs()`
- Used by fg/bg/jobs builtin commands

### builtins.cpp/builtins.h
- **Builtin Registry**: `init_builtins()` populates command map
- **Builtin Check**: `is_builtin()` - checks if command is a builtin
- **Builtin Execution**: `execute_builtin()` - handles I/O redirection for builtins
- **Implemented Commands**:
  - `exit [code]` - Exit the shell
  - `echo <args>` - Print arguments
  - `type <cmd>` - Show command type
  - `pwd` - Print working directory
  - `cd [dir]` - Change directory
  - `history [n]` - View/manage command history
  - `fg [job]` - Bring job to foreground
  - `bg [job]` - Resume job in background
  - `jobs` - List background jobs
  - `help` - Show help message

### completion.cpp/completion.h
- **Command Generator**: `command_generator()` - generates completion matches
- **Completion Handler**: `command_completion()` - readline integration
- Provides tab completion for builtins and PATH executables

### heredoc.cpp/heredoc.h
- **Heredoc Reader**: `read_heredoc()` - reads multi-line input for `<<DELIMITER`
- Stores content in `RedirectionConfig` for later use

### utils.cpp/utils.h
- **String Utilities**: `split_string()`, `trim()`
- **Path Resolution**: `find_executable_in_path()` - searches PATH for commands
- **Executable Discovery**: `get_all_executables()` - lists all PATH executables

## Building

```bash
make clean
make
```

This compiles all source files separately and links them into the `shell` executable.

## Running

```bash
./shell
```

Or use the Makefile:

```bash
make run
```

## Key Data Structures

### RedirectionConfig
```cpp
struct RedirectionConfig {
    std::string stdout_file, stderr_file, stdin_file;
    std::string heredoc_delimiter, heredoc_content;
    bool stdout_append, stderr_append, use_heredoc;
    int stdin_pipe;
};
```

### ASTNode
```cpp
struct ASTNode {
    NodeType type;  // COMMAND, PIPELINE, BACKGROUND, SEQUENCE
    std::string command;
    std::vector<std::string> args;
    RedirectionConfig redir;
    std::vector<std::unique_ptr<ASTNode>> children;
};
```

### Job
```cpp
struct Job {
    int job_id;
    pid_t pgid;
    std::string command;
    bool stopped, background;
    std::vector<pid_t> pids;
};
```

## Advantages of Modular Structure

1. **Separation of Concerns**: Each module has a clear, single responsibility
2. **Easier Testing**: Individual modules can be tested independently
3. **Faster Compilation**: Only changed files need recompilation
4. **Better Code Organization**: Related functionality is grouped together
5. **Improved Maintainability**: Bugs can be isolated to specific modules
6. **Team Development**: Multiple developers can work on different modules
7. **Code Reusability**: Modules can be reused in other projects

## Dependencies Between Modules

```
main.cpp
  ├── shell.h (init_shell, run_shell)
  └── builtins.h (init_builtins)

shell.cpp
  ├── parser.h (process_command)
  ├── executor.h (execute commands)
  ├── job_control.h (Job struct, jobs vector)
  └── utils.h (trim)

parser.cpp
  ├── executor.h (process_command for substitution)
  ├── heredoc.h (read_heredoc)
  └── utils.h (trim, split_string)

executor.cpp
  ├── parser.h (parse_to_ast, RedirectionConfig, ASTNode)
  ├── builtins.h (is_builtin, execute_builtin)
  ├── job_control.h (add_job, jobs vector)
  └── utils.h (find_executable_in_path)

builtins.cpp
  ├── job_control.h (find_job, jobs vector)
  ├── shell.h (shell_pgid, shell_is_interactive)
  └── utils.h (find_executable_in_path)

completion.cpp
  ├── builtins.h (builtins map)
  └── utils.h (get_all_executables)
```
