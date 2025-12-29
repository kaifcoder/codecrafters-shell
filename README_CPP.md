# C++ Shell Implementation

This is a full-featured POSIX-compliant shell written in C++, translated and enhanced from the Python implementation. It includes all the features of the original plus advanced job control, signal handling, command substitution, heredocs, and AST-based parsing.

## Features

### Core Shell Features
- **Command Execution**: Execute both builtin and external programs
- **PATH Resolution**: Automatically finds executables in PATH directories
- **Interactive REPL**: Read-Eval-Print-Loop with command history
- **AST-based Parsing**: Abstract syntax tree for robust command parsing

### Builtin Commands
- `exit [code]` - Exit the shell with optional exit code
- `echo <args>` - Print arguments to stdout
- `type <cmd>` - Show command type (builtin or path to executable)
- `pwd` - Print current working directory
- `cd [dir]` - Change directory (supports `~`, `-`, and relative/absolute paths)
- `history [n|-r file|-w file|-a file]` - View/manage command history
- `jobs` - List background jobs
- `fg [job]` - Bring job to foreground
- `bg [job]` - Resume stopped job in background

### I/O Redirection
- `>` or `1>` - Redirect stdout (overwrite)
- `>>` or `1>>` - Redirect stdout (append)
- `2>` - Redirect stderr (overwrite)
- `2>>` - Redirect stderr (append)
- `<` - Redirect stdin from file
- `<<DELIMITER` - Heredoc (inline input)
- Supports both spaced (`cmd > file`) and inline (`cmd>file`) syntax

### Pipelines
- Unlimited command chaining with `|` operator
- Supports mixing builtin and external commands
- Proper file descriptor management for multi-stage pipelines

### Job Control
- **Background Jobs**: `command &` - Run command in background
- **Job Management**: `jobs`, `fg`, `bg` commands
- **Process Groups**: Proper PGID management for job control
- **Job Notification**: Automatic notification when background jobs complete/stop

### Signal Handling
- **SIGINT (Ctrl+C)**: Interrupts foreground job, not the shell
- **SIGTSTP (Ctrl+Z)**: Suspends foreground job
- **SIGCHLD**: Automatic reaping of background processes
- **Proper Terminal Control**: Jobs receive proper terminal access

### Command Substitution
- `$(command)` - Execute command and substitute output
- Nested substitution support
- Works in arguments and quoted strings

### Heredocs
- `<<DELIMITER` - Multi-line input redirection
- Interactive prompt for heredoc content
- Proper delimiter matching

### Command History
- Persistent history stored in `~/.shell_history` or `$HISTFILE`
- Up/down arrow navigation through command history
- Limited to 500 entries
- History management: `-r` (read), `-w` (write), `-a` (append)

### Tab Completion
- Command name completion for builtins and PATH executables
- Smart completion with multiple match display
- Cross-platform readline support

## Building

### Prerequisites
- C++17 compatible compiler (g++, clang++)
- GNU readline library
- Make

#### Installing readline on macOS:
```bash
brew install readline
```

#### Installing readline on Linux:
```bash
# Debian/Ubuntu
sudo apt-get install libreadline-dev

# Fedora/RHEL
sudo dnf install readline-devel
```

### Compilation
```bash
# Build the shell
make

# Or compile manually
g++ -std=c++17 -o shell app/main.cpp -lreadline
```

## Running

```bash
# Using the provided script
./your_program_cpp.sh

# Or run directly after building
./shell
```

## Usage Examples

### Basic Commands
```bash
$ pwd
/home/user/project

$ echo Hello World
Hello World

$ type echo
echo is a shell builtin

$ type ls
ls is /bin/ls
```

### I/O Redirection
```bash
# Redirect stdout
$ echo "Hello" > output.txt

# Append to file
$ echo "World" >> output.txt

# Redirect stderr
$ invalid_command 2> errors.txt

# Redirect both
$ command > out.txt 2> err.txt

# Input redirection
$ cat < input.txt
```

### Heredocs
```bash
$ cat <<EOF
> This is line 1
> This is line 2
> EOF
This is line 1
This is line 2

$ grep pattern <<END
> test pattern here
> another line
> END
test pattern here
```

### Pipelines
```bash
# Two-command pipeline
$ echo "hello world" | wc -w
2

# Multi-stage pipeline
$ cat file.txt | grep pattern | wc -l

# Mix builtins and external commands
$ pwd | cat | cat
```

### Job Control
```bash
# Run in background
$ sleep 100 &
[1] 12345

# List jobs
$ jobs
[1]  Running                 sleep 100 &

# Bring to foreground
$ fg 1
sleep 100

# Suspend with Ctrl+Z
^Z
[1]+ Stopped                 sleep 100

# Resume in background
$ bg 1
[1]+ sleep 100 &

# Multiple background jobs
$ sleep 50 &
[2] 12346
$ sleep 75 &
[3] 12347
$ jobs
[1]  Running                 sleep 100 &
[2]  Running                 sleep 50 &
[3]  Running                 sleep 75 &
```

### Command Substitution
```bash
# Simple substitution
$ echo "Current directory: $(pwd)"
Current directory: /home/user/project

# Nested substitution
$ echo "Files: $(ls $(pwd))"
Files: file1.txt file2.txt

# In command arguments
$ cat $(echo "file.txt")

# Multiple substitutions
$ echo "$(whoami) at $(hostname)"
user at localhost
```

### Signal Handling
```bash
# Ctrl+C interrupts foreground job, not shell
$ sleep 100
^C
$ echo "Shell still running"
Shell still running

# Ctrl+Z suspends job
$ cat
^Z
[1]+ Stopped                 cat
$ jobs
[1]  Stopped                 cat
$ fg 1
cat
```

### Tab Completion
```bash
$ ec<TAB>      # Completes to "echo "
$ gre<TAB>     # Shows: grep  grepdiff  (if both exist)
```

### Command History
```bash
$ history 5                    # Show last 5 commands
$ history -w ~/my_history.txt  # Write history to file
$ history -r ~/my_history.txt  # Read history from file
$ history -a ~/my_history.txt  # Append new commands only
```

## Implementation Details

### Architecture
- **Single-file implementation**: All code in [app/main.cpp](app/main.cpp)
- **Readline integration**: Uses GNU readline for input, history, and completion
- **Fork/exec model**: External commands executed via fork() + execv()
- **Pipe chains**: Multi-command pipelines using pipe() and dup2()
- **Job control**: Process groups (PGID) for proper terminal management
- **AST parsing**: Abstract syntax tree for robust command interpretation

### Key Components
- `ASTNode`: Represents parsed command structure (COMMAND, PIPELINE, BACKGROUND, SEQUENCE)
- `parse_to_ast()`: Converts input string to AST
- `execute_ast_node()`: Recursively executes AST nodes
- `expand_command_substitution()`: Processes $(...) substitutions
- `parse_arguments()`: Tokenizes input with quote and escape handling
- `parse_pipeline()`: Splits input on pipe operators
- `parse_redirection()`: Extracts I/O redirection operators and heredocs
- `command_completion()`: Tab completion callback for readline
- `find_executable_in_path()`: Searches PATH for executables
- `sigchld_handler()`: Reaps background processes and updates job status
- `init_shell()`: Initializes terminal modes and signal handlers

### Job Control Implementation
- **Process Groups**: Each job gets unique PGID for terminal control
- **Terminal Management**: Only foreground job owns terminal (tcsetpgrp)
- **Signal Isolation**: Signals only affect job process group, not shell
- **Job Tracking**: Global jobs vector tracks all background/stopped jobs
- **Status Updates**: SIGCHLD handler automatically updates job states

### Signal Handling Strategy
- **SIGINT**: Shell ignores, foreground job receives (Ctrl+C)
- **SIGTSTP**: Shell ignores, foreground job receives (Ctrl+Z)
- **SIGCHLD**: Shell handles to reap zombies and update job status
- **Reset in Children**: Child processes reset to default signal handlers

### Command Substitution Algorithm
1. Scan input for `$(` patterns outside single quotes
2. Find matching `)` with depth tracking for nesting
3. Extract command string between delimiters
4. Fork child process with output pipe
5. Execute command in child and capture stdout
6. Replace `$(...)` with command output (trailing newline removed)
7. Continue parsing with expanded string

### Heredoc Processing
1. Detect `<<DELIMITER` in redirection parsing
2. Enter interactive mode with `>` prompt
3. Read lines until exact delimiter match
4. Store accumulated content in RedirectionConfig
5. During execution, create pipe and write content
6. Redirect stdin to pipe read end

### Memory Management
- Uses RAII principles for automatic cleanup
- Smart pointers (unique_ptr) for AST nodes
- Proper file descriptor closing in all code paths
- Readline memory freed after each command
- Job vector automatically cleans completed jobs

### Platform Support
- **Linux**: Full support with GNU readline
- **macOS**: Full support with libedit (readline compatibility)
- **Unix-like systems**: Should work with POSIX compliance

## Differences from Python Version

### Advantages
- **Performance**: Significantly faster startup and execution
- **Memory**: Lower memory footprint
- **Native**: No interpreter required

### Considerations
- Requires compilation step
- More verbose code compared to Python
- Platform-specific compilation (readline library)

## Development

### Clean Build
```bash
make clean
make
```

### Debug Build
```bash
g++ -std=c++17 -g -o shell app/main.cpp -lreadline
```

## Technical Highlights

- ✅ Complete feature parity with Python implementation + advanced features
- ✅ POSIX-compliant command execution and job control
- ✅ Robust argument parsing with quote/escape handling
- ✅ Full I/O redirection support (stdout, stderr, stdin, append modes, heredocs)
- ✅ Unlimited pipeline depth
- ✅ Background job execution with `&` operator
- ✅ Job control (jobs, fg, bg commands)
- ✅ Proper signal handling (SIGINT, SIGTSTP, SIGCHLD)
- ✅ Terminal control and process group management
- ✅ Command substitution with `$(...)` syntax
- ✅ Heredoc support with `<<DELIMITER`
- ✅ AST-based parsing for complex command structures
- ✅ Persistent command history
- ✅ Interactive tab completion
- ✅ Cross-platform readline support
- ✅ Proper process management and cleanup
- ✅ Signal handling (Ctrl+C, Ctrl+D, Ctrl+Z)
- ✅ Automatic job status notification

## Advanced Features Comparison

| Feature | Python Shell | C++ Shell |
|---------|--------------|-----------|
| Basic Commands | ✅ | ✅ |
| I/O Redirection | ✅ | ✅ |
| Pipelines | ✅ | ✅ |
| History | ✅ | ✅ |
| Tab Completion | ✅ | ✅ |
| **Background Jobs (&)** | ❌ | ✅ |
| **Job Control (fg/bg)** | ❌ | ✅ |
| **Signal Handling** | ❌ | ✅ |
| **Command Substitution** | ❌ | ✅ |
| **Heredocs (<<)** | ❌ | ✅ |
| **AST Parsing** | ❌ | ✅ |

## License

This implementation follows the same license as the original Python version.
