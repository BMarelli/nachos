/// Copy content of a file to another file

#include "syscall.h"

#define ARGC_ERROR "Error: missing argument"
#define OPEN_ERROR "Error: could not open file."
#define CREATE_ERROR "Error: could not create file."

int main(int argc, char *argv[]) {
    if (argc < 3) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId fid = Open(argv[1]);
    if (fid < 0) {
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    if (Create(argv[2]) < 0) {
        Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId fid2 = Open(argv[2]);
    if (fid2 < 0) {
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    char buffer[128];
    int size;
    while ((size = Read(buffer, sizeof(buffer), fid)) > 0) {
        Write(buffer, size, fid2);
    }

    Close(fid);
    Close(fid2);
    Exit(0);
}