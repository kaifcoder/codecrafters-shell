import sys
import os
import subprocess
import shlex
import readline
from dataclasses import dataclass
from typing import Optional, Any


@dataclass
class RedirectionConfig:
    """Configuration for I/O redirection."""
    stdout_file: Optional[str] = None
    stderr_file: Optional[str] = None
    stdout_append: bool = False
    stderr_append: bool = False
    stdin_pipe: Optional[Any] = None


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


def history_command(*_):
    """Display command history."""
    history_length = readline.get_current_history_length()
    for i in range(1, history_length + 1):
        item = readline.get_history_item(i)
        if item:
            print(f"    {i}  {item}")


BUILTINS = {
    "exit": exit_command,
    "echo": echo_command,
    "type": type_command,
    "pwd": pwd_command,
    "cd": cd_command,
    "history": history_command,
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
    print("$ " + readline.get_line_buffer(), end='', flush=True)


def setup_readline():
    """Configure readline for tab completion and history."""
    readline.set_completer(completer)
    # Bind tab to complete - try multiple methods for compatibility
    if readline.__doc__ and 'libedit' in readline.__doc__:
        # macOS uses libedit instead of GNU readline
        readline.parse_and_bind("bind ^I rl_complete")
    else:
        readline.parse_and_bind("tab: complete")
    # Disable default filename completion behavior
    readline.set_completer_delims(' \t\n;')
    # Set custom display for multiple matches
    readline.set_completion_display_matches_hook(display_matches)
    
    # Enable history with limit
    readline.set_history_length(500)  # Limit to 500 entries
    
    # Load history from file if it exists
    history_file = os.path.expanduser("~/.shell_history")
    if os.path.exists(history_file):
        try:
            readline.read_history_file(history_file)
        except Exception:
            pass
    
    return history_file


def save_history(history_file):
    """Save command history to file, limiting to last 500 entries."""
    try:
        # Append history and limit file size
        readline.append_history_file(readline.get_current_history_length(), history_file)
        # Truncate to keep only last 500 entries
        readline.write_history_file(history_file)
    except (AttributeError, OSError):
        # Fallback if append_history_file not available
        try:
            readline.write_history_file(history_file)
        except Exception:
            pass


def should_add_to_history(command):
    """Determine if a command should be added to history."""
    if not command or not command.strip():
        return False
    
    # Don't add consecutive duplicates
    history_length = readline.get_current_history_length()
    if history_length > 0:
        last_item = readline.get_history_item(history_length)
        if last_item == command:
            return False
    
    return True



def open_redirect_file(filename, append_mode):
    """Open a file for redirection with appropriate mode."""
    mode = 'a' if append_mode else 'w'
    return open(filename, mode) if filename else None


def run_external_program(command, args, redir: RedirectionConfig):
    """Execute an external program found in PATH."""
    executable_path = find_executable_in_path(command)
    
    if not executable_path:
        print(f"{command}: command not found")
        return None
    
    try:
        stdout_handle = open_redirect_file(redir.stdout_file, redir.stdout_append)
        stderr_handle = open_redirect_file(redir.stderr_file, redir.stderr_append)
        
        try:
            result = subprocess.run(
                [command] + args,
                executable=executable_path,
                stdin=redir.stdin_pipe,
                stdout=stdout_handle if stdout_handle else subprocess.PIPE if redir.stdin_pipe is not None else None,
                stderr=stderr_handle,
                capture_output=False
            )
            return result
        finally:
            if stdout_handle:
                stdout_handle.close()
            if stderr_handle:
                stderr_handle.close()
    except Exception as e:
        print(f"{command}: {e}")
        return None


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
    original_stdin = sys.stdin
    stdout_file_handle = None
    stderr_file_handle = None
    
    try:
        if redir.stdout_file:
            stdout_file_handle = open_redirect_file(redir.stdout_file, redir.stdout_append)
            sys.stdout = stdout_file_handle
        if redir.stderr_file:
            stderr_file_handle = open_redirect_file(redir.stderr_file, redir.stderr_append)
            sys.stderr = stderr_file_handle
        if redir.stdin_pipe:
            sys.stdin = redir.stdin_pipe
        
        BUILTINS[command](*args)
    finally:
        sys.stdout = original_stdout
        sys.stderr = original_stderr
        sys.stdin = original_stdin
        if stdout_file_handle:
            stdout_file_handle.close()
        if stderr_file_handle:
            stderr_file_handle.close()


def parse_pipeline(usr_input):
    """Parse input for pipeline operator (|)."""
    # Find pipe operator outside of quotes
    parts = []
    current = []
    in_quote = None
    escaped = False
    
    for char in usr_input:
        if escaped:
            current.append(char)
            escaped = False
            continue
        
        if char == '\\':
            escaped = True
            current.append(char)
            continue
        
        if char in ('"', "'"):
            if in_quote == char:
                in_quote = None
            elif not in_quote:
                in_quote = char
            current.append(char)
        elif char == '|' and not in_quote:
            parts.append(''.join(current).strip())
            current = []
        else:
            current.append(char)
    
    if current:
        parts.append(''.join(current).strip())
    
    return parts


def execute_pipeline(commands):
    """Execute a pipeline of commands."""
    if len(commands) == 1:
        process_command(commands[0])
        return
    
    # Parse all commands
    parsed_commands = []
    for cmd_str in commands:
        try:
            parts = shlex.split(cmd_str)
        except ValueError:
            parts = cmd_str.split()
        
        parts, redir = parse_redirection(parts)
        if not parts:
            continue
        
        command, args = parts[0], parts[1:]
        parsed_commands.append((command, args, redir))
    
    if not parsed_commands:
        return
    
    # Execute pipeline chain
    processes = []
    prev_output = None  # Can be string or pipe
    
    for i, (cmd, args, redir) in enumerate(parsed_commands):
        is_first = i == 0
        is_last = i == len(parsed_commands) - 1
        
        if cmd in BUILTINS:
            # Handle builtin in pipeline
            if is_first and not is_last:
                # First command - capture output
                from io import StringIO
                original_stdout = sys.stdout
                captured = StringIO()
                
                try:
                    sys.stdout = captured
                    execute_builtin(cmd, args, redir)
                    sys.stdout = original_stdout
                    prev_output = captured.getvalue()
                finally:
                    sys.stdout = original_stdout
            elif not is_first and not is_last:
                # Middle builtin - read from prev, capture output
                from io import StringIO
                original_stdout = sys.stdout
                original_stdin = sys.stdin
                captured = StringIO()
                
                try:
                    if isinstance(prev_output, str):
                        sys.stdin = StringIO(prev_output)
                    sys.stdout = captured
                    execute_builtin(cmd, args, redir)
                    prev_output = captured.getvalue()
                finally:
                    sys.stdout = original_stdout
                    sys.stdin = original_stdin
            else:
                # Last builtin - just read from prev
                from io import StringIO
                original_stdin = sys.stdin
                try:
                    if isinstance(prev_output, str):
                        sys.stdin = StringIO(prev_output)
                    execute_builtin(cmd, args, redir)
                finally:
                    sys.stdin = original_stdin
        else:
            # External command
            executable_path = find_executable_in_path(cmd)
            if not executable_path:
                print(f"{cmd}: command not found")
                # Clean up any running processes
                for proc in processes:
                    proc.terminate()
                return
            
            stdin_source = None
            stdout_dest = None
            
            if not is_first:
                # Not first - read from previous
                if isinstance(prev_output, str):
                    # Previous was a builtin, use PIPE to write string
                    stdin_source = subprocess.PIPE
                else:
                    # Previous was external process
                    stdin_source = prev_output
            
            if not is_last:
                # Not last - pipe to next
                stdout_dest = subprocess.PIPE
            
            proc = subprocess.Popen(
                [cmd] + args,
                executable=executable_path,
                stdin=stdin_source,
                stdout=stdout_dest,
                stderr=None
            )
            
            processes.append(proc)
            
            # If previous output was a string (from builtin), write it
            if isinstance(prev_output, str) and proc.stdin:
                proc.stdin.write(prev_output.encode())
                proc.stdin.close()
            
            # Close previous process stdout if it exists and it's a pipe
            if prev_output and not isinstance(prev_output, str) and hasattr(prev_output, 'close'):
                prev_output.close()
            
            prev_output = proc.stdout
    
    # Wait for all processes to complete
    for proc in processes:
        proc.wait()


def process_command(usr_input):
    """Parse and execute a single command."""
    # Check for pipeline first
    pipeline_parts = parse_pipeline(usr_input)
    if len(pipeline_parts) > 1:
        execute_pipeline(pipeline_parts)
        return
    
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
    history_file = setup_readline()
    
    try:
        while True:
            try:
                sys.stdout.write("$ ")
                sys.stdout.flush()

                usr_input = input().strip()
                if usr_input:
                    # Check if we should skip adding to history
                    if not should_add_to_history(usr_input):
                        # Remove it from history (input() adds it automatically)
                        history_length = readline.get_current_history_length()
                        if history_length > 0:
                            readline.remove_history_item(history_length - 1)
                    
                    process_command(usr_input)
                    
            except EOFError:
                break
            except KeyboardInterrupt:
                print()
                continue
    finally:
        # Save history on exit
        save_history(history_file)


if __name__ == "__main__":
    main()
