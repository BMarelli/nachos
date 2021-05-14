#include "lib.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char str[] = "Argumentos incorrectos. rm [origen]";
        Write(str, strlen(str), CONSOLE_OUTPUT);
        return 1;
    }
    Remove(argv[1]);
    return 0;
}