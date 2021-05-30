#include "lib.c"

int main(int argc, char *argv[])
{

    if (argc != 3) {
        puts2("Error: cantidad de argumentos incorrecta.\n");
        return 1;
    }

    OpenFileId origen = Open(argv[1]);
    if (origen == -1) {
        puts2("Error: el archivo de origen no existe.\n");
        return 1;
    }

    if (Create(argv[2]) == -1) {
        puts2("Error: no se pudo crear el archivo destino\n");
        return 1;
    }

    OpenFileId destino = Open(argv[2]);

    char buf[128];
    while (Read(buf, sizeof(buf), origen) > 0)
        Write(buf, strlen(buf), destino);

    Close(origen);
    Close(destino);

    return 0;
}