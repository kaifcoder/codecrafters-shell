import sys

RESERVED = {
    "exit": lambda code=0, *_: sys.exit(int(code)),
    "echo": lambda *args: print(" ".join(args)),
    "type": lambda x: print(
        f"{x} is a shell builtin" if x in RESERVED else f"{x}: not found"
    ),
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
