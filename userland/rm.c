/// Delete a file or directory.

#include "lib.c"
#include "syscall.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("Error: missing argument.\n");

        return 1;
    }

    int deleted = 0;
    for (unsigned i = 1; i < argc; i++) {
        if (Remove(argv[i]) < 0) {
            puts("Error: failed to remove file from directory: ");
            puts(argv[i]);
            puts("\n");

            deleted = 1;
        }
    }

    return deleted;
}
