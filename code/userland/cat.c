#include "lib.h"

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    char str[] = "Argumento incorrecto";
    Write(str, strlen(str), CONSOLE_OUTPUT);
    return 1;
  }

  OpenFileId file = Open(argv[1]);
  if (file == -1) {
    char str[] = "El archivo no existe.";
    Write(str, strlen(str), CONSOLE_OUTPUT);
  }
  char buf[128];

  while (Read(buf, sizeof(buf), file) > 0)
    Write(buf, strlen(buf), CONSOLE_OUTPUT);

  Close(file);

  return 0;
}