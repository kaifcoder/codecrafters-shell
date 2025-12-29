import sys

def echo(message: str) -> None:
    sys.stdout.write(message + "\n")

def type_check(command: str):
    builtin_commands = ["echo", "exit"]
    if command in builtin_commands:
        return f"{command} is a shell builtin"
    else:
        return f"{command}: not found"

def main():
    sys.stdout.write("$ ")
    input_string = input()
    exit(0) if input_string == "exit" else None
    if input_string.startswith("echo "):
        echo(input_string[5:])
    elif input_string.startswith("type "):
        command = input_string[5:]
        result = type_check(command)
        sys.stdout.write(result + "\n")
    else:
        sys.stdout.write(f"{input_string}: command not found\n")


if __name__ == "__main__":
    while True:
        main()
