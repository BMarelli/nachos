#include "lib.c"

int main(int argc, char *argv[])
{
  if (argc != 2) {
    puts2("Error: cantidad de argumentos incorrecta.\n");
    return 1;
  }

  OpenFileId file = Open(argv[1]);
  if (file == -1) {
    puts2("Error: el archivo no existe.\n");
    return 1;
  }

  char buf[128];
  while (Read(buf, sizeof(buf), file) > 0) {
    puts2(buf);
  }
  puts2("\n");

  Close(file);

  return 0;
}