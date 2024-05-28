/// Outputs arguments entered on the command line.

#include "lib.c"
#include "syscall.h"

int main(int argc, char *argv[]) {
    for (unsigned i = 1; i < argc; i++) {
        if (i != 1) {
            puts(" ");
        }

        puts(argv[i]);
    }

    puts("\n");

    return 0;
}
