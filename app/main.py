import sys
import os
import subprocess
import shlex


def find_executable_in_path(cmd):
    """Search for an executable in PATH directories."""
    path_env = os.environ.get("PATH", "")
    if not path_env:
        return None
    
    path_dirs = path_env.split(os.pathsep)
    
    for directory in path_dirs:
        # Skip if directory doesn't exist
        if not os.path.isdir(directory):
            continue
            
        file_path = os.path.join(directory, cmd)
        
        # Check if file exists and has execute permissions
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
    # Check if it's a builtin
    if cmd in BUILTINS:
        print(f"{cmd} is a shell builtin")
        return
    
    # Search for executable in PATH
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
    if dir == "-":
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


def run_external_program(command, args, stdout_file=None, stderr_file=None):
    """Execute an external program found in PATH."""
    executable_path = find_executable_in_path(command)
    
    if not executable_path:
        print(f"{command}: command not found")
        return
    
    # Execute the program with the command name as argv[0] followed by arguments
    try:
        stdout_handle = open(stdout_file, 'w') if stdout_file else None
        stderr_handle = open(stderr_file, 'w') if stderr_file else None
        
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
    operator is '>', '1>', or '2>' if redirection found, else None
    """
    # Spaced operators: > file, 1> file, 2> file
    if token in ('>', '1>', '2>'):
        return token, next_token, 2 if next_token else 1
    
    # Inline operators: >file, 1>file, 2>file
    for op in ('2>', '1>', '>'):
        if token.startswith(op):
            return op, token[len(op):], 1
    
    return None, None, 0


def parse_redirection(parts):
    """Parse command parts for stdout (> or 1>) and stderr (2>) redirection."""
    stdout_file = None
    stderr_file = None
    filtered_parts = []
    i = 0

    while i < len(parts):
        next_token = parts[i + 1] if i + 1 < len(parts) else None
        operator, filename, consumed = extract_redirection_info(parts[i], next_token)
        
        if operator:
            if operator in ('>', '1>') and filename:
                stdout_file = filename
            elif operator == '2>' and filename:
                stderr_file = filename
            i += consumed
        else:
            filtered_parts.append(parts[i])
            i += 1

    return filtered_parts, stdout_file, stderr_file


def execute_builtin(command, args, stdout_file, stderr_file):
    """Execute a builtin command with optional stdout/stderr redirection."""
    original_stdout = sys.stdout
    original_stderr = sys.stderr
    
    try:
        if stdout_file:
            sys.stdout = open(stdout_file, 'w')
        if stderr_file:
            sys.stderr = open(stderr_file, 'w')
        
        BUILTINS[command](*args)
    finally:
        if stdout_file and sys.stdout != original_stdout:
            sys.stdout.close()
            sys.stdout = original_stdout
        if stderr_file and sys.stderr != original_stderr:
            sys.stderr.close()
            sys.stderr = original_stderr


def process_command(usr_input):
    """Parse and execute a single command."""
    # Parse input with quote handling
    try:
        parts = shlex.split(usr_input)
    except ValueError:
        # Handle unclosed quotes
        parts = usr_input.split()
    
    # Parse for redirections
    parts, stdout_file, stderr_file = parse_redirection(parts)
    
    if not parts:
        return
        
    command = parts[0]
    args = parts[1:]
    
    if command in BUILTINS:
        execute_builtin(command, args, stdout_file, stderr_file)
    else:
        run_external_program(command, args, stdout_file, stderr_file)


def main():
    """Main shell loop."""
    while True:
        try:
            sys.stdout.write("$ ")
            sys.stdout.flush()

            usr_input = input().strip()
            if not usr_input:
                continue
            
            process_command(usr_input)
                
        except EOFError:
            break
        except KeyboardInterrupt:
            print()
            continue


if __name__ == "__main__":
    main()
