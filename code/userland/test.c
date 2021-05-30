#include "../userprog/syscall.h"

int main(int argc, char* argv[]) {

    char *args[] = {"userland/test", "test.txt, test2.txt", (void *) 0};
    SpaceId proc1 = Exec("userland/cp", args);
    int i = Join(proc1);
    if (i == 1)
        Write("Retorno bien el Join\n",22,1);
}