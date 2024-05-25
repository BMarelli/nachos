/// Outputs the contents of a file to the console.

#include "lib.c"
#include "syscall.h"

#define ARGC_ERROR "Error: missing argument.\n"
#define OPEN_ERROR "Error: could not open file: "

#define BUFFER_SIZE 128

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts(ARGC_ERROR);
        Exit(1);
    }

    char buffer[BUFFER_SIZE];

    for (unsigned i = 1; i < argc; i++) {
        OpenFileId fid = Open(argv[i]);
        if (fid < 0) {
            puts(OPEN_ERROR);
            puts(argv[i]);
            Exit(1);
        }

        int size;
        while ((size = Read(buffer, sizeof buffer, fid)) > 0) {
            Write(buffer, size, CONSOLE_OUTPUT);
        }

        Write("\n", 1, CONSOLE_OUTPUT);

        Close(fid);
    }

    Exit(0);
}
