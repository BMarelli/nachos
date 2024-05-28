#include "lib.c"
#include "syscall.h"

#define NULL ((void *)0)
#define PROMPT "--> "
#define BUFFER_SIZE 60

int main(void) {
    char ch, buffer[BUFFER_SIZE];

    for (;;) {
        puts(PROMPT);

        int i = 0;
        while ((ch = getchar()) != '\n' && i < BUFFER_SIZE - 1) {
            buffer[i++] = ch;
        }
        buffer[i] = '\0';

        if (i > 0) {
            SpaceId newProc = Exec(buffer, NULL);
            if (newProc < 0) {
                puts("Error: failed to execute command.\n");

                continue;
            }

            int status = Join(newProc);
            puts("Process exited with status: ");
            puti(status);
            puts("\n");
        }
    }

    return -1;
}
