#include "lib.c"
#include "syscall.h"

int main() {
    Mkdir("dir1");
    Mkdir("dir2");

    Ls(NULL);

    Cd("dir1");
    Mkdir("foo");

    Ls(NULL);

    Cd(NULL);
    Mkdir("dir3");

    Ls(NULL);

    // TODO: rmdir and ls
}
