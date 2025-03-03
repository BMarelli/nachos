/// Creates files specified on the command line.

#include "lib.c"
#include "syscall.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("Error: missing argument.\n");

        return 1;
    }

    int success = 1;
    for (unsigned i = 1; i < argc; i++) {
        if (Create(argv[i]) < 0) {
            puts("Error: failed to create file: ");
            puts(argv[i]);
            puts("\n");

            success = 0;
        }
    }

    return !success;
}
