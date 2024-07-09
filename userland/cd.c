/// Change the working directory to the directory specified on the command line.

#include "lib.c"
#include "syscall.h"

int main(int argc, char *argv[]) {
    if (argc > 2) {
        puts("Error: too many arguments.\n");

        return 1;
    }

    char *path = "";
    if (argc == 2) path = argv[1];

    return Cd(path);
}