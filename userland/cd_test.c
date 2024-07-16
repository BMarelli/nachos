#include "lib.c"
#include "syscall.h"

    int main(int argc, char* argv[]) {
        if (argc > 2) {
            puts("Error: too many arguments.\n");

            return 1;
        }

        char* name = argv[1];
        if (Mkdir(name) < 0) {
            puts("Error: failed to create directory: ");
            puts(name);
            puts("\n");

            return 0;
        }

        Cd(name);
        Mkdir("testing");
        Mkdir("hello");

        return Ls(NULL);
    }