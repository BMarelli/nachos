#include "thread_test_join.hh"

#include <stdio.h>

#include <cstdlib>

#include "system.hh"

#define NUM_THREADS 10
#define MIN_ITERATIONS 50
#define MAX_ITERATIONS 200

void ThreadJoin(void *n) {
    for (int i = 0; i < rand() % (MAX_ITERATIONS - MIN_ITERATIONS) + MIN_ITERATIONS; i++) {
        printf("`%s`: %d\n", currentThread->GetName(), i);

        currentThread->Yield();
    }

    printf("Thread %ld finished\n", (long)n);
}

void ThreadTestJoin() {
    Thread *threads[NUM_THREADS];

    for (long i = 0; i < NUM_THREADS; i++) {
        char *name = new char[16];
        sprintf(name, "Thread %ld", i);
        threads[i] = new Thread(name, true);

        threads[i]->Fork(ThreadJoin, (void *)i);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i]->Join();
    }

    printf("All threads finished\n");
}
