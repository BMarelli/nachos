/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_prod_cons.hh"

#include <stdio.h>

#include <cstdlib>

#include "condition.hh"
#include "lock.hh"
#include "system.hh"

Lock *lock = new Lock();
Condition *empty = new Condition(lock);
Condition *full = new Condition(lock);

static const int NUM_PRODUCERS = 20;
static const int NUM_CONSUMERS = 2;

static const long MAX_TO_PRODUCE = 100;
static const long MAX_EXTRA_CAPACITY = 10;

int buffer, capacity;

long produced = 0;
long consumed = 0;

#define YIELD currentThread->Yield()

void Producer(void *n) {
    for (int i = 0; i < (long)n; i++) {
        YIELD;

        lock->Acquire();

        YIELD;

        while (buffer == capacity) {
            empty->Wait();
        }

        YIELD;

        buffer++;
        produced++;
        printf("%s produced %d.\n", currentThread->GetName(), buffer);

        YIELD;

        full->Signal();

        YIELD;

        lock->Release();

        YIELD;
    }
}

void Consumer(void *n) {
    for (int i = 0; i < (long)n; i++) {
        YIELD;

        lock->Acquire();

        YIELD;

        while (buffer == 0) {
            full->Wait();
        }

        YIELD;

        buffer--;
        consumed++;
        printf("%s consumed %d.\n", currentThread->GetName(), buffer);

        YIELD;

        empty->Signal();

        YIELD;

        lock->Release();

        YIELD;
    }
}

void ThreadTestProdCons() {
    long toProduce = 0, toConsume = 0;

    // Launch a new thread for each producer.
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        printf("Launching producer %u.\n", i);

        char *name = new char[16];
        sprintf(name, "Producer %u", i);

        Thread *thread = new Thread(name);

        long n = rand() % (MAX_TO_PRODUCE - toProduce + 1);

        toProduce += n;
        thread->Fork(Producer, (void *)n);
    }

    // Launch a new thread for each consumer.
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        printf("Launching consumer %u.\n", i);

        char *name = new char[16];
        sprintf(name, "Consumer %u", i);

        Thread *thread = new Thread(name);

        long n = rand() % (toProduce - toConsume + 1);

        toConsume += n;
        thread->Fork(Consumer, (void *)n);
    }

    capacity = rand() % (MAX_EXTRA_CAPACITY - (toProduce - toConsume) + 1) + toProduce - toConsume;

    while (produced < toProduce || consumed < toConsume) {
        currentThread->Yield();
    }

    printf("All producers and consumers finished. Final buffer is %d (should be %ld).\n", buffer, toProduce - toConsume);
}
