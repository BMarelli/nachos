#include "lib.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts2("Error: cantidad de argumentos incorrecta.\n");
        return 1;
    }

    Remove(argv[1]);

    return 0;
}