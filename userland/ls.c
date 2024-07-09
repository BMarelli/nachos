/// Print the content of the directory specified on the command line.

#include "lib.c"
#include "syscall.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return Ls("");
    }

    int error = 0;
    for (unsigned i = 1; i < argc; i++) {
        // TODO: if argc > 2, print the name of the directory before the content
        // e.g.: ls dir1 dir2
        // dir1:
        // file1 file2 file3
        // dir2:
        // file4 file5 file6

        if (Ls(argv[i]) < 0) {
            puts("Error: cannot access '");
            puts(argv[i]);
            puts("': No such file or directory\n");

            error = 1;
        }
    }

    return error;
}