import sys

def echo(message: str) -> None:
    sys.stdout.write(message + "\n")

def main():
    sys.stdout.write("$ ")
    input_string = input()
    exit(0) if input_string == "exit" else None
    if input_string.startswith("echo "):
        echo(input_string[5:])
    else:
        sys.stdout.write(f"{input_string}: command not found\n")


if __name__ == "__main__":
    while True:
        main()
