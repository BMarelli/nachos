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

void Producer(void *n) {
    for (int i = 0; i < (long)n; i++) {
        lock->Acquire();

        while (buffer == capacity) {
            empty->Wait();
        }

        buffer++;
        produced++;
        printf("%s produced %d.\n", currentThread->GetName(), buffer);

        full->Signal();

        lock->Release();
    }
}

void Consumer(void *n) {
    for (int i = 0; i < (long)n; i++) {
        lock->Acquire();

        while (buffer == 0) {
            full->Wait();
        }

        buffer--;
        consumed++;
        printf("%s consumed %d.\n", currentThread->GetName(), buffer);

        empty->Signal();

        lock->Release();
    }
}

void ThreadTestProdCons() {
    Thread *producerThreads[NUM_PRODUCERS], *consumerThreads[NUM_CONSUMERS];
    long toProduce = 0, toConsume = 0;

    // Launch a new thread for each producer.
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        printf("Launching producer %u.\n", i);

        char *name = new char[16];
        sprintf(name, "Producer %u", i);

        producerThreads[i] = new Thread(name, true);

        long n = rand() % (MAX_TO_PRODUCE - toProduce + 1);

        toProduce += n;
        producerThreads[i]->Fork(Producer, (void *)n);
    }

    // Launch a new thread for each consumer.
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        printf("Launching consumer %u.\n", i);

        char *name = new char[16];
        sprintf(name, "Consumer %u", i);

        consumerThreads[i] = new Thread(name, true);

        long n = rand() % (toProduce - toConsume + 1);

        toConsume += n;
        consumerThreads[i]->Fork(Consumer, (void *)n);
    }

    capacity = rand() % (MAX_EXTRA_CAPACITY - (toProduce - toConsume) + 1) + toProduce - toConsume;

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producerThreads[i]->Join();
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumerThreads[i]->Join();
    }

    printf("All producers and consumers finished. Final buffer is %d (should be %ld).\n", buffer, toProduce - toConsume);
}
