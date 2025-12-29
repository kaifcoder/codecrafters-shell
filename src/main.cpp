#include "shell.h"
#include "builtins.h"

int main() {
    init_shell();
    init_builtins();
    run_shell();
    return 0;
}
