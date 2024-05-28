#include "thread_test_garden_sem.hh"

#include <stdio.h>

#include "semaphore.hh"
#include "system.hh"

static const unsigned NUM_TURNSTILES = 2;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static int count;

static Semaphore *sem = new Semaphore(1);

static void Turnstile(void *n_) {
    unsigned *n = (unsigned *)n_;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
        sem->P();

        int temp = count;
        currentThread->Yield();
        count = temp + 1;

        sem->V();
    }

    printf("Turnstile %u finished. Count is now %u.\n", *n, count);

    delete n;
}

void ThreadTestGardenSem() {
    Thread *threads[NUM_TURNSTILES];

    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char[16];
        sprintf(name, "Turnstile %u", i);
        unsigned *n = new unsigned;
        *n = i;
        threads[i] = new Thread(name, true);
        threads[i]->Fork(Turnstile, (void *)n);
    }

    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        threads[i]->Join();
    }

    printf("All turnstiles finished. Final count is %u (should be %u).\n", count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
