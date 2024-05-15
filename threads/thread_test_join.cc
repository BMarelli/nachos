#include "thread_test_join.hh"

#include <stdio.h>

#include <cstdlib>

#include "system.hh"

void ThreadJoin(void *n) {
    for (int i = 0; i < rand() % 20 + 5; i++) {
        printf("Thread %ld: %d\n", (long)n, i);

        currentThread->Yield();
    }

    printf("Thread %ld finished\n", (long)n);
}

void ThreadTestJoin() {
    Thread *threads[5];

    for (long i = 0; i < 5; i++) {
        threads[i] = new Thread("thread", true);
        threads[i]->Fork(ThreadJoin, (void *)i);
    }

    for (int i = 0; i < 5; i++) {
        threads[i]->Join();
    }

    printf("All threads finished\n");
}
