/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_garden.hh"

#include <stdio.h>

#include "system.hh"

static const unsigned NUM_TURNSTILES = 2;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static int count;

static void Turnstile(void *n_) {
    unsigned *n = (unsigned *)n_;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
        int temp = count;
        count = temp + 1;
        currentThread->Yield();
    }

    printf("Turnstile %u finished. Count is now %u.\n", *n, count);

    delete n;
}

void ThreadTestGarden() {
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
