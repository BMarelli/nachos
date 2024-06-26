#include "lib.c"
#include "syscall.h"

#define THREAD_COUNT 3  // Number of threads to create (must be less than 10)
#define FILE_NAME "fs_test.txt"
#define BUFFER_SIZE 256

int ThreadMain(char *argv0) {
    puts("info: main thread started.\n");

    if (Create(FILE_NAME) < 0) {
        puts("Error: failed to create file.\n");

        return 1;
    }

    puts("info: file created: ");
    puts(FILE_NAME);
    puts("\n");

    puts("info: starting child processes.\n");

    SpaceId childProcesses[THREAD_COUNT];

    for (int i = 1; i <= THREAD_COUNT; i++) {
        char idStr[2];
        idStr[0] = i + '0';
        idStr[1] = '\0';

        char *args[] = {argv0, idStr, NULL};
        childProcesses[i - 1] = Exec(argv0, args);
        if (childProcesses[i - 1] < 0) {
            puts("Error: failed to start child process.\n");

            return 1;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (Join(childProcesses[i]) < 0) {
            puts("Error: failed to join child process.\n");

            return 1;
        }
    }

    puts("info: all child processes finished.\n");

    OpenFileId fid = Open(FILE_NAME);
    if (fid < 0) {
        puts("Error: failed to open file.\n");

        return 1;
    }

    puts("info: file contents:\n");

    char buffer[BUFFER_SIZE];

    int bytesRead;
    while ((bytesRead = Read(buffer, sizeof buffer, fid)) > 0) {
        if (Write(buffer, bytesRead, CONSOLE_OUTPUT) < 0) {
            puts("Error: failed to write to console.\n");

            return 1;
        }
    }

    Close(fid);

    return 1;
}

int ThreadReader(int threadId) {
    OpenFileId fid = Open(FILE_NAME);
    if (fid < 0) {
        puts("Error: failed to open file.\n");

        return 1;
    }

    char buffer[BUFFER_SIZE];
    int bytesRead;
    while ((bytesRead = Read(buffer, BUFFER_SIZE, fid)) > 0) {
        if (Write(buffer, bytesRead, CONSOLE_OUTPUT) < 0) {
            puts("Error: failed to write to console.\n");

            return 1;
        }
    }

    Close(fid);

    return 1;
}

int ThreadWriter(int threadId) {
    static char buffer[] =
        "Thread _: _\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris sed ultricies neque. Sed facilisis augue libero, eu "
        "rhoncus leo.\n";
    buffer[7] = threadId + '0';

    OpenFileId fid = Open(FILE_NAME);
    if (fid < 0) {
        puts("Error: failed to open file for writing.\n");

        return 1;
    }

    for (int i = 0; i < 10; i++) {
        buffer[10] = i + '0';

        if (Write(buffer, sizeof buffer, fid) < 0) {
            puts("Error: failed to write to file.\n");

            return 1;
        }
    }

    Close(fid);

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("Error: missing argument.\n");

        return 1;
    }

    int threadId = atoi(argv[1]);

    if (threadId < 0 || threadId > THREAD_COUNT) {
        puts("Error: invalid thread ID.\n");

        return 1;
    }

    if (threadId == 0) {
        ThreadMain(argv[0]);
    } else {
        int threadType = threadId % 2;

        switch (threadType) {
            case 0:
                return ThreadReader(threadId);
            case 1:
                return ThreadWriter(threadId);
            default:
                puts("Error: invalid thread type.\n");

                return 1;
        }
    }

    return 0;
}
