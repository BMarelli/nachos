/// Outputs the contents of a file to the console.

#include "lib.c"
#include "syscall.h"

#define BUFFER_SIZE 128

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("Error: missing argument.\n");

        return 1;
    }

    char buffer[BUFFER_SIZE];

    for (unsigned i = 1; i < argc; i++) {
        OpenFileId fid = Open(argv[i]);
        if (fid < 0) {
            puts("Error: failed to open file: ");
            puts(argv[i]);
            puts("\n");

            return 1;
        }

        int size;
        while ((size = Read(buffer, sizeof buffer, fid)) > 0) {
            if (Write(buffer, size, CONSOLE_OUTPUT) < 0) {
                puts("Error: failed to write to console.\n");

                return 1;
            }
        }

        Close(fid);
    }

    return 0;
}
