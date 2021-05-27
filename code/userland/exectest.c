#include "lib.h"

int main() {
    char *argv[10];
    argv[0] = "userland/exectarget";
    argv[1] = "arg1";
    argv[2] = "arg2";
    argv[3] = ((void *) 0);
    const SpaceId newProc = Exec("userland/exectarget", argv);

    Join(newProc);
    puts2("\nEnd!\n");

    return 0;
}