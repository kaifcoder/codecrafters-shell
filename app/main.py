import sys
import os

def type_command(cmd):
    """Handle the type builtin command."""
    # Check if it's a builtin
    if cmd in RESERVED:
        print(f"{cmd} is a shell builtin")
        return
    
    # Search through PATH directories
    path_env = os.environ.get("PATH", "")
    path_dirs = path_env.split(os.pathsep)
    
    for directory in path_dirs:
        # Skip if directory doesn't exist
        if not os.path.isdir(directory):
            continue
            
        file_path = os.path.join(directory, cmd)
        
        # Check if file exists and has execute permissions
        if os.path.isfile(file_path) and os.access(file_path, os.X_OK):
            print(f"{cmd} is {file_path}")
            return
    
    # Not found
    print(f"{cmd}: not found")

RESERVED = {
    "exit": lambda code=0, *_: sys.exit(int(code)),
    "echo": lambda *args: print(" ".join(args)),
    "type": lambda cmd, *_: type_command(cmd),
}

def main():
    while True:
        sys.stdout.write("$ ")
        sys.stdout.flush()

        usr_input = input().split()
        command = usr_input[0]
        args = usr_input[1:]
        if command in RESERVED:
            RESERVED[command](*args)
        else:
            print(f"{command}: command not found")


if __name__ == "__main__":
    main()
