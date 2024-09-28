/// Print the content of the directory specified on the command line.

#include "lib.c"
#include "syscall.h"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return ListDirectoryContents(NULL);
    }

    int error = 0;
    for (unsigned i = 1; i < argc; i++) {
        if (argc > 2) {
            puts(argv[i]);
            puts(":\n");
        }

        if (ListDirectoryContents(argv[i]) < 0) {
            puts("Error: cannot access '");
            puts(argv[i]);
            puts("': No such file or directory\n");

            error = 1;
        }
    }

    return error;
}
