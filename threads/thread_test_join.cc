#include "thread_test_join.hh"

#include <stdio.h>

#include <cstdlib>

#include "system.hh"

#define NUM_THREADS 10
#define MIN_ITERATIONS 50
#define MAX_ITERATIONS 200

void ThreadJoin(void *n) {
    for (int i = 0; i < rand() % (MAX_ITERATIONS - MIN_ITERATIONS) + MIN_ITERATIONS; i++) {
        printf("Thread %ld: %d\n", (long)n, i);

        currentThread->Yield();
    }

    printf("Thread %ld finished\n", (long)n);
}

void ThreadTestJoin() {
    Thread *threads[NUM_THREADS];

    for (long i = 0; i < NUM_THREADS; i++) {
        threads[i] = new Thread("thread", true);
        threads[i]->Fork(ThreadJoin, (void *)i);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i]->Join();
    }

    printf("All threads finished\n");
}
