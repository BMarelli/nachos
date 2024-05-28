#include "thread_test_garden_sem.hh"

#include <stdio.h>

#include "semaphore.hh"
#include "system.hh"

static const unsigned NUM_TURNSTILES = 2;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static bool done[NUM_TURNSTILES];
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
    done[*n] = true;
    delete n;
}

void ThreadTestGardenSem() {
    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char[16];
        sprintf(name, "Turnstile %u", i);
        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);
        t->Fork(Turnstile, (void *)n);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n", count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
