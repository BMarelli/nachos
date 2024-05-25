/// Copy content of a file to another file

#include "lib.c"
#include "syscall.h"

#define BUFFER_SIZE 128

int main(int argc, char *argv[]) {
    if (argc < 3) {
        puts("Error: missing argument.\n");

        return 1;
    }

    OpenFileId fromFid = Open(argv[1]);
    if (fromFid < 0) {
        puts("Error: failed to open file: ");
        puts(argv[1]);
        puts("\n");

        return 1;
    }

    if (Create(argv[2]) < 0) {
        puts("Error: failed to create file: ");
        puts(argv[2]);
        puts("\n");

        return 1;
    }

    OpenFileId toFid = Open(argv[2]);
    if (toFid < 0) {
        puts("Error: failed to open file: ");
        puts(argv[2]);
        puts("\n");

        return 1;
    }

    char buffer[BUFFER_SIZE];

    int size;
    while ((size = Read(buffer, sizeof buffer, fromFid)) > 0) {
        if (Write(buffer, size, toFid) < 0) {
            puts("Error: failed to write to file: ");
            puts(argv[2]);
            puts("\n");

            return 1;
        }
    }

    Close(fromFid);
    Close(toFid);

    return 0;
}
