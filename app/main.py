import sys
import os
import subprocess
import shlex
import readline
from dataclasses import dataclass
from typing import Optional


@dataclass
class RedirectionConfig:
    """Configuration for I/O redirection."""
    stdout_file: Optional[str] = None
    stderr_file: Optional[str] = None
    stdout_append: bool = False
    stderr_append: bool = False


def find_executable_in_path(cmd):
    """Search for an executable in PATH directories."""
    path_env = os.environ.get("PATH", "")
    if not path_env:
        return None
    
    for directory in path_env.split(os.pathsep):
        if not os.path.isdir(directory):
            continue
            
        file_path = os.path.join(directory, cmd)
        if os.path.isfile(file_path) and os.access(file_path, os.X_OK):
            return file_path
    
    return None


def exit_command(code=0, *_):
    """Exit the shell with the given code."""
    sys.exit(int(code))


def echo_command(*args):
    """Print arguments to stdout."""
    print(" ".join(args))


def type_command(cmd, *_):
    """Check if a command is a builtin or find it in PATH."""
    if cmd in BUILTINS:
        print(f"{cmd} is a shell builtin")
        return
    
    executable_path = find_executable_in_path(cmd)
    if executable_path:
        print(f"{cmd} is {executable_path}")
    else:
        print(f"{cmd}: not found")


def pwd_command(*_):
    """Print the current working directory."""
    print(os.getcwd())


def cd_command(dir=None):
    """Change the current working directory."""
    target_dir = dir or os.path.expanduser("~")
    
    if dir == "~":
        target_dir = os.path.expanduser("~")
    elif dir == "-":
        target_dir = os.environ.get("OLDPWD", os.getcwd())
    
    try:
        old_pwd = os.getcwd()
        os.chdir(target_dir)
        os.environ["OLDPWD"] = old_pwd
    except Exception:
        print(f"cd: {dir}: No such file or directory")


BUILTINS = {
    "exit": exit_command,
    "echo": echo_command,
    "type": type_command,
    "pwd": pwd_command,
    "cd": cd_command,
}


def get_all_executables():
    """Get all executables from PATH directories."""
    executables = set()
    path_env = os.environ.get("PATH", "")
    
    for directory in path_env.split(os.pathsep):
        if not os.path.isdir(directory):
            continue
        
        try:
            for filename in os.listdir(directory):
                file_path = os.path.join(directory, filename)
                if os.path.isfile(file_path) and os.access(file_path, os.X_OK):
                    executables.add(filename)
        except PermissionError:
            continue
    
    return executables


def completer(text, state):
    """Tab completion function for readline."""
    line = readline.get_line_buffer()
    tokens = line.lstrip().split()
    
    # If we're completing the first word (command name)
    if not tokens or (len(tokens) == 1 and not line.endswith(' ')):
        # Get all available commands (builtins + executables)
        builtins = list(BUILTINS.keys())
        executables = list(get_all_executables())
        options = builtins + executables
        
        # Filter options that match the text
        matches = [cmd + ' ' for cmd in options if cmd.startswith(text)]
        matches.sort()
        
        if state < len(matches):
            return matches[state]
    
    return None


def display_matches(substitution, matches, longest_match_length):
    """Custom display function for completion matches."""
    print()
    # Remove trailing spaces from matches for display
    display_matches = [match.rstrip() for match in matches]
    print("  ".join(display_matches))
    # Redisplay the prompt and current line
    print(readline.get_line_buffer(), end='', flush=True)


def setup_readline():
    """Configure readline for tab completion."""
    readline.set_completer(completer)
    readline.parse_and_bind("tab: complete")
    # Disable default filename completion behavior
    readline.set_completer_delims(' \t\n;')
    # Set custom display for multiple matches
    readline.set_completion_display_matches_hook(display_matches)



def open_redirect_file(filename, append_mode):
    """Open a file for redirection with appropriate mode."""
    mode = 'a' if append_mode else 'w'
    return open(filename, mode) if filename else None


def run_external_program(command, args, redir: RedirectionConfig):
    """Execute an external program found in PATH."""
    executable_path = find_executable_in_path(command)
    
    if not executable_path:
        print(f"{command}: command not found")
        return
    
    try:
        stdout_handle = open_redirect_file(redir.stdout_file, redir.stdout_append)
        stderr_handle = open_redirect_file(redir.stderr_file, redir.stderr_append)
        
        try:
            subprocess.run(
                [command] + args,
                executable=executable_path,
                stdout=stdout_handle,
                stderr=stderr_handle
            )
        finally:
            if stdout_handle:
                stdout_handle.close()
            if stderr_handle:
                stderr_handle.close()
    except Exception as e:
        print(f"{command}: {e}")


def extract_redirection_info(token, next_token):
    """Extract redirection operator and filename from token(s).
    
    Returns: (operator, filename, tokens_consumed)
    """
    SPACED_OPS = ('>', '>>', '1>', '1>>', '2>', '2>>')
    INLINE_OPS = ('2>>', '1>>', '>>', '2>', '1>', '>')
    
    if token in SPACED_OPS:
        return token, next_token, 2 if next_token else 1
    
    for op in INLINE_OPS:
        if token.startswith(op):
            return op, token[len(op):], 1
    
    return None, None, 0


def apply_redirection(operator, filename, redir: RedirectionConfig):
    """Apply redirection configuration based on operator and filename."""
    if not filename:
        return
    
    if operator in ('>', '1>'):
        redir.stdout_file = filename
        redir.stdout_append = False
    elif operator in ('>>', '1>>'):
        redir.stdout_file = filename
        redir.stdout_append = True
    elif operator == '2>':
        redir.stderr_file = filename
        redir.stderr_append = False
    elif operator == '2>>':
        redir.stderr_file = filename
        redir.stderr_append = True


def parse_redirection(parts):
    """Parse command parts for I/O redirection."""
    redir = RedirectionConfig()
    filtered_parts = []
    i = 0

    while i < len(parts):
        next_token = parts[i + 1] if i + 1 < len(parts) else None
        operator, filename, consumed = extract_redirection_info(parts[i], next_token)
        
        if operator:
            apply_redirection(operator, filename, redir)
            i += consumed
        else:
            filtered_parts.append(parts[i])
            i += 1

    return filtered_parts, redir


def execute_builtin(command, args, redir: RedirectionConfig):
    """Execute a builtin command with optional I/O redirection."""
    original_stdout = sys.stdout
    original_stderr = sys.stderr
    stdout_file_handle = None
    stderr_file_handle = None
    
    try:
        if redir.stdout_file:
            stdout_file_handle = open_redirect_file(redir.stdout_file, redir.stdout_append)
            sys.stdout = stdout_file_handle
        if redir.stderr_file:
            stderr_file_handle = open_redirect_file(redir.stderr_file, redir.stderr_append)
            sys.stderr = stderr_file_handle
        
        BUILTINS[command](*args)
    finally:
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        if stdout_file_handle:
            stdout_file_handle.close()
        if stderr_file_handle:
            stderr_file_handle.close()


def process_command(usr_input):
    """Parse and execute a single command."""
    try:
        parts = shlex.split(usr_input)
    except ValueError:
        parts = usr_input.split()
    
    parts, redir = parse_redirection(parts)
    
    if not parts:
        return
        
    command, args = parts[0], parts[1:]
    
    if command in BUILTINS:
        execute_builtin(command, args, redir)
    else:
        run_external_program(command, args, redir)


def main():
    """Main shell loop."""
    setup_readline()
    
    while True:
        try:
            sys.stdout.write("$ ")
            sys.stdout.flush()

            usr_input = input().strip()
            if usr_input:
                process_command(usr_input)
                
        except EOFError:
            break
        except KeyboardInterrupt:
            print()
            continue


if __name__ == "__main__":
    main()
