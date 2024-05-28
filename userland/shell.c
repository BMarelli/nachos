#include "lib.c"
#include "stdbool.h"
#include "syscall.h"

#define MAX_LINE_SIZE 60
#define MAX_ARG_COUNT 32
#define ARG_SEPARATOR ' '

#define NULL ((void *)0)

#define PROMPT "--> "

static inline void WriteError(const char *description) {
    puts("Error: ");
    puts(description);
    puts("\n");
}

static unsigned ReadLine(char *buffer, unsigned size) {
    if (size == 0) {
        return 0;
    }

    char ch;

    int i = 0;
    while ((ch = getchar()) != '\n' && i < size - 1) {
        buffer[i++] = ch;
    }
    buffer[i] = '\0';

    return i;
}

static bool PrepareArguments(char *line, char **argv, unsigned argvSize) {
    if (line == NULL || argv == NULL || argvSize == 0) {
        return false;
    }

    // Skip leading spaces.
    int i = 0;
    while (line[i] == ARG_SEPARATOR && line[i] != '\0') {
        i++;
    }

    unsigned argc = 1;
    argv[0] = &line[0];

    // Traverse the whole line and replace spaces between arguments by null
    // characters, so as to be able to treat each argument as a standalone
    // string.
    // TODO: support arguments with spaces by using quotes / escaping.
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == ARG_SEPARATOR) {
            if (argc == argvSize - 1) {
                // The maximum of allowed arguments is exceeded, and
                // therefore the size of `argv` is too.  Note that 1 is
                // decreased in order to leave space for the NULL at the end.
                return false;
            }

            line[i] = '\0';
            argv[argc] = &line[i + 1];
            argc++;

            // Skip following spaces.
            while (line[i + 1] == ARG_SEPARATOR) {
                i++;
            }
        }
    }

    argv[argc] = (char *)NULL;

    return true;
}

int main(void) {
    char line[MAX_LINE_SIZE];
    char *argv[MAX_ARG_COUNT];

    for (;;) {
        puts(PROMPT);

        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE);
        if (lineSize == 0) {
            continue;
        }

        if (!PrepareArguments(line, argv, MAX_ARG_COUNT)) {
            WriteError("too many arguments.");

            continue;
        }

        bool parallel = argv[0][0] == '&';

        char *name = argv[0] + (parallel ? 1 : 0);  // Skip '&' if present.

        const SpaceId newProc = Exec(name, argv);
        if (newProc < 0) {
            WriteError("failed to execute command.");

            continue;
        }

        if (!parallel) {
            const int status = Join(newProc);
            if (status < 0) {
                WriteError("failed to join process.");
            }
        }
    }

    // Never reached.
    return -1;
}
