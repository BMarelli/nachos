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

static int PrepareArguments(char *line, char **argv, unsigned argvSize) {
    if (line == NULL || argv == NULL || argvSize == 0) {
        return -1;
    }

    int i = 0;

    // Skip leading spaces.
    while (line[i] == ARG_SEPARATOR && line[i] != '\0') i++;

    if (line[i] == '\0') return 0;

    argv[0] = &line[i];
    unsigned argc = 1;

    // Traverse the whole line and replace spaces between arguments by null
    // characters, so as to be able to treat each argument as a standalone
    // string.
    // TODO: support arguments with spaces by using quotes / escaping.
    for (; line[i] != '\0'; i++) {
        if (line[i] == ARG_SEPARATOR) {
            argc++;

            if (argc == argvSize) {
                // The maximum of allowed arguments is exceeded, and
                // therefore the size of `argv` is too. Note that we need
                // to leave 1 slot for the NULL terminator.
                return -1;
            }

            line[i] = '\0';

            // Skip following spaces.
            while (line[i + 1] == ARG_SEPARATOR) i++;

            // If only spaces followed, then there were no more arguments.
            if (line[i + 1] == '\0') {
                argc--;
            } else {
                argv[argc - 1] = &line[i + 1];
            }
        }
    }

    argv[argc] = (char *)NULL;

    return argc;
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

        int res = PrepareArguments(line, argv, MAX_ARG_COUNT);
        switch (res) {
            case 0:
                // Empty line.
                continue;
            case -1:
                WriteError("too many arguments.");
                continue;
        }

        if (strcmp(argv[0], "cd") == 0) {
            if (res > 2) {
                WriteError("too many arguments.");

                continue;
            }

            char *target;
            if (res == 1)
                target = NULL;
            else
                target = argv[1];

            if (ChangeDirectory(target) < 0) WriteError("failed to change directory.");
        } else {
            bool parallel = argv[0][0] == '&';

            char *name = argv[0] + (parallel ? 1 : 0);  // Skip '&' if present.

            const SpaceId newProc = Exec(name, argv, parallel);
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
    }

    // Never reached.
    return -1;
}
