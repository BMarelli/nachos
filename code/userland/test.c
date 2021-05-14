#include "syscall.h"
#include "lib.h"

int main(int argc, char* argv[]) {
    // Create("test_lib.txt");
    // OpenFileId o = Open("test_lib.txt");
    // const char *str = "success\nhola\ncomo\nestas";
    // int len = strlen(str);
    // Write(str, len, o);
    // Close(o);

    char *args[] = {"test_lib.txt, test_lib2.txt"};
    SpaceId proc1 = Exec("userland/cp", args);
    int i = Join(proc1);
    if (i == 1)
        Write("Retorno bien el Join\n",29,1);
}