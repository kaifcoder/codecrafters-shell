import sys


def main():
    sys.stdout.write("$ ")
    input_string = input()
    exit(0) if input_string == "exit" else None
    sys.stdout.write(f"{input_string}: command not found\n")


if __name__ == "__main__":
    while True:
        main()
