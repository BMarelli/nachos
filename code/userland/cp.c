#include "lib.h"

int main(int argc, char *argv[])
{
    Write(argv[0], strlen(argv[0]), CONSOLE_OUTPUT);
    Write(argv[1], strlen(argv[1]), CONSOLE_OUTPUT);
    return 1;


    if (argc != 3)
    {
        char str[] = "Argumento incorrecto. cp [origen] [destino]";
        Write(str, strlen(str), CONSOLE_OUTPUT);
    }

    OpenFileId origen = Open(argv[1]);
    if (origen == -1) {
        char str[] = "El archivo no existe.";
        Write(str, strlen(str), CONSOLE_OUTPUT);
    }
    Create(argv[2]);
    OpenFileId destino = Open(argv[2]);
    char buf[128];

    while (Read(buf, sizeof(buf), origen) > 0)
        Write(buf, strlen(buf), destino);

    Close(origen);
    Close(destino);
    return 0;
}