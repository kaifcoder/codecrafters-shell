import sys
import os
import subprocess


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
    if dir == "-":
        target_dir = os.environ.get("OLDPWD", os.getcwd())
    
    try:
        old_pwd = os.getcwd()
        os.chdir(target_dir)
        os.environ["OLDPWD"] = old_pwd
    except Exception as e:
        print(f"cd: {e}")

BUILTINS = {
    "exit": exit_command,
    "echo": echo_command,
    "type": type_command,
    "pwd": pwd_command,
    "cd": cd_command,
}


def run_external_program(command, args):
    """Execute an external program found in PATH."""
    executable_path = find_executable_in_path(command)
    
    if not executable_path:
        print(f"{command}: command not found")
        return
    
    # Execute the program with the command name as argv[0] followed by arguments
    try:
        subprocess.run([command] + args, executable=executable_path)
    except Exception as e:
        print(f"{command}: {e}")


def main():
    """Main shell loop."""
    while True:
        try:
            sys.stdout.write("$ ")
            sys.stdout.flush()

            usr_input = input().strip()
            if not usr_input:
                continue
                
            parts = usr_input.split()
            command = parts[0]
            args = parts[1:]
            
            if command in BUILTINS:
                BUILTINS[command](*args)
            else:
                run_external_program(command, args)
                
        except EOFError:
            break
        except KeyboardInterrupt:
            print()
            continue


if __name__ == "__main__":
    main()
