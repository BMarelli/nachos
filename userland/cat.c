/// Outputs the contents of a file to the console.

#include "syscall.h"

#define ARGC_ERROR "Error: missing argument"
#define OPEN_ERROR "Error: could not open file."

int main(int argc, char *argv[]) {
    if (argc < 2) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    for (unsigned i = 1; i < argc; i++) {
        OpenFileId fid = Open(argv[i]);
        if (fid < 0) {
            Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }

        char buffer[128];
        int size;
        while ((size = Read(buffer, sizeof(buffer), fid)) > 0) {
            Write(buffer, size, CONSOLE_OUTPUT);
        }
        Write("\n", sizeof("\n"), CONSOLE_OUTPUT);

        Close(fid);
        Exit(0);
    }
}