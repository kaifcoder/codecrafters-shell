[![progress-banner](https://backend.codecrafters.io/progress/shell/5667e953-cbfc-47b7-aba9-7c52fa68f5c8)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

# Build Your Own Shell - Python Implementation

This is a Python implementation of a POSIX-compliant shell for the ["Build Your Own Shell" Challenge](https://app.codecrafters.io/courses/shell/overview).

A feature-rich, interactive shell that supports command execution, builtin commands, I/O redirection, pipelines, command history, and tab completion.

> 🚀 **Also Available**: [C++ Implementation](README_CPP.md) with advanced features including job control (&, fg, bg), signal handling (SIGINT, SIGTSTP), command substitution ($(...)), heredocs (<<), and AST-based parsing!

## Features

### 🚀 Core Functionality
- **REPL (Read-Eval-Print Loop)** - Interactive command-line interface
- **Command Execution** - Run external programs from PATH
- **Builtin Commands** - Native shell commands (exit, echo, type, pwd, cd, history)
- **Quote Handling** - Proper parsing of single and double quotes
- **Error Handling** - Graceful handling of invalid commands and errors

### 📂 I/O Redirection
- **Stdout Redirection** - `>` and `1>` (overwrite), `>>` and `1>>` (append)
- **Stderr Redirection** - `2>` (overwrite), `2>>` (append)
- **Combined Redirection** - Redirect stdout and stderr simultaneously
- **Works with both builtins and external commands**

### 🔗 Pipeline Support
- **Multi-command Pipelines** - Chain unlimited commands with `|`
- **Mixed Command Types** - Seamlessly pipe between builtins and external programs
- **Example**: `cat file.txt | grep pattern | sort | uniq | wc -l`

### 📜 Command History
- **Persistent History** - Commands saved to file (respects `$HISTFILE`)
- **History Navigation** - Up/Down arrow keys to browse command history
- **History Builtin** - View and manage command history
  - `history` - Display all history
  - `history N` - Display last N entries
  - `history -r <file>` - Read history from file
  - `history -w <file>` - Write history to file
  - `history -a <file>` - Append new commands to file
- **Smart Tracking** - Only appends new commands on subsequent `-a` calls
- **Configurable Limit** - Maximum 500 entries (configurable)

### ⌨️ Tab Completion
- **Command Completion** - Auto-complete builtin and external commands
- **Smart Suggestions** - Shows multiple matches when ambiguous
- **Cross-platform** - Works on both GNU readline and macOS libedit

## Builtin Commands

| Command | Description | Examples |
|---------|-------------|----------|
| `exit [code]` | Exit the shell with optional exit code | `exit`, `exit 0` |
| `echo <args>` | Print arguments to stdout | `echo hello world` |
| `type <cmd>` | Show if command is builtin or external | `type echo`, `type ls` |
| `pwd` | Print current working directory | `pwd` |
| `cd [dir]` | Change directory | `cd /tmp`, `cd ~`, `cd -` |
| `history [N]` | Display command history | `history`, `history 10` |
| `history -r <file>` | Read history from file | `history -r ~/.bash_history` |
| `history -w <file>` | Write history to file | `history -w /tmp/hist` |
| `history -a <file>` | Append new history to file | `history -a /tmp/hist` |

## Usage Examples

### Basic Commands
```bash
$ echo Hello, World!
Hello, World!

$ pwd
/home/user

$ cd /tmp
$ pwd
/tmp
```

### I/O Redirection
```bash
# Redirect stdout
$ echo hello > output.txt
$ cat output.txt
hello

# Redirect stderr
$ ls nonexistent 2> errors.txt

# Append output
$ echo world >> output.txt
$ cat output.txt
hello
world

# Redirect both
$ command > stdout.txt 2> stderr.txt
```

### Pipelines
```bash
# Simple pipeline
$ echo "test data" | cat
test data

# Multi-stage pipeline
$ ls | grep .py | wc -l
5

# Complex data processing
$ cat data.txt | grep pattern | sort | uniq | wc -l
42
```

### Tab Completion
```bash
$ ec<TAB>
$ echo   # Auto-completed

$ e<TAB><TAB>
echo  exit  env   # Shows all matching commands

$ echo he<TAB>
$ echo hello   # Completes and adds space
```

### Command History
```bash
$ echo first
$ echo second
$ history
    1  echo first
    2  echo second
    3  history

# Use Up/Down arrows to navigate
$ <UP>    # Recalls: history
$ <UP>    # Recalls: echo second
$ <DOWN>  # Recalls: history

# Save history
$ history -w /tmp/my_history
$ history -a /tmp/my_history  # Append only new commands
```

## Getting Started

### Prerequisites
- Python 3.6+
- `uv` package manager (recommended)
- Unix-like environment (Linux, macOS, WSL)

### Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd codecrafters-shell-python
```

2. Ensure `uv` is installed:
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

3. Run the shell:
```bash
./your_program.sh
```

### Configuration

Set the `HISTFILE` environment variable to customize history file location:
```bash
export HISTFILE=~/.my_shell_history
./your_program.sh
```

## Implementation Details

### Architecture
- **Modular Design** - Separate functions for parsing, execution, and I/O
- **Dataclasses** - Type-safe configuration objects for redirection
- **Readline Integration** - Native readline for history and completion
- **Process Management** - Proper subprocess handling with pipes

### Key Components
- **`parse_redirection()`** - Parses I/O redirection operators from command
- **`parse_pipeline()`** - Splits commands by pipe operator
- **`execute_pipeline()`** - Chains multiple commands with proper I/O
- **`completer()`** - Tab completion for commands
- **`history_command()`** - Full history management
- **`setup_readline()`** - Configure readline for both GNU and libedit

### File Structure
```
.
├── app/
│   └── main.py          # Main shell implementation
├── your_program.sh      # Entry point script
├── codecrafters.yml     # CodeCrafters configuration
└── README.md           # This file
```

## Development

### Running Tests
```bash
codecrafters test
```

### Submitting Solutions
```bash
git add .
git commit -m "feat: implement feature"
git push origin master
```

## Features Checklist

- [x] REPL with prompt
- [x] External program execution
- [x] Builtin commands (exit, echo, type, pwd, cd)
- [x] PATH resolution
- [x] Quote handling (single and double quotes)
- [x] I/O redirection (>, >>, 2>, 2>>)
- [x] Pipelines (multiple commands)
- [x] Tab completion
- [x] Command history
- [x] History navigation (arrow keys)
- [x] History persistence (HISTFILE)
- [x] History management (-r, -w, -a flags)

## Technical Highlights

### Cross-Platform Compatibility
- Detects GNU readline vs libedit (macOS)
- Handles different key bindings and behaviors
- OS-agnostic path handling (`os.pathsep`)

### Smart History Management
- Tracks last write position per file for `-a` flag
- Prevents duplicate appends
- Configurable history limits
- Empty command filtering

### Robust Pipeline Execution
- Handles builtin-to-external and external-to-builtin pipes
- String buffering for builtin output
- Proper file descriptor management
- Process cleanup on errors

## Acknowledgments

This project is part of the [CodeCrafters](https://codecrafters.io) "Build Your Own Shell" challenge.

## License

MIT License - feel free to use this code for learning purposes.

---

**Note**: If you're viewing this repo on GitHub, head over to [codecrafters.io](https://codecrafters.io) to try the challenge yourself!
