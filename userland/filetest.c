/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!

#include "lib.c"
#include "syscall.h"

#define TEST_FILE "test.txt"
#define HELLO_WORLD "Hello, world!\n"

int helloWorld(const char *filename) {
    if (Create(filename) < 0) {
        puts("Error: failed to create file: ");
        puts(filename);
        puts("\n");

        return 1;
    }

    OpenFileId fid = Open(filename);
    if (fid < 0) {
        puts("Error: failed to open file: ");
        puts(filename);
        puts("\n");

        return 1;
    }

    if (Write(HELLO_WORLD, sizeof HELLO_WORLD, fid) < 0) {
        puts("Error: failed to write to file: ");
        puts(filename);
        puts("\n");

        return 1;
    }

    Close(fid);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return helloWorld(TEST_FILE);
    }

    return helloWorld(argv[1]);
}
