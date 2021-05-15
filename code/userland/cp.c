#include "lib.h"

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        char str[] = "Argumento incorrecto. cp [origen] [destino]";
        Write(str, strlen(str), CONSOLE_OUTPUT);
        return 1;
    }

    OpenFileId origen = Open(argv[1]);
    if (origen == -1) {
        char str[] = "El archivo de origen no existe.";
        Write(str, strlen(str), CONSOLE_OUTPUT);
        return 1;
    }

    if (Create(argv[2]) == -1) {
        char str[] = "Error creando el archivo destino.";
        Write(str, strlen(str), CONSOLE_OUTPUT);
        return 1;
    }

    char buf[128];
    OpenFileId destino = Open(argv[2]);

    while (Read(buf, sizeof(buf), origen) > 0)
        Write(buf, strlen(buf), destino);

    Close(origen);
    Close(destino);

    return 0;
}