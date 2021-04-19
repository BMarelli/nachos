/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef SEMAPHORE_TEST
#include "semaphore.hh"
Semaphore *sem = new Semaphore("Semaphore", 3);
#endif

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    #ifdef SEMAPHORE_TEST
    DEBUG('s', "<thread %s> llamado a P()\n", name);
    sem->P();
    #endif

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);

    #ifdef SEMAPHORE_TEST
    DEBUG('s', "<thread %s> llamado a V()\n", name);
    sem->V();
    #endif
}

void Priority1(void *id) {
    printf("Esto deberia ejecutarse con mas prioridad, cuando se termine esto siguen las demas\n");
    for(int i=0; i < 10; i++) {
        printf("Iteracion Priority1 %d\n", i);
        sleep(1);
        currentThread->Yield();
    }
}

void Priority2(void* _id) {
    int id = *(int*) _id;
    printf("Entrando a priority2 %d\n", id);
    for(int i = 0; i < 10; i++) {
        printf("Iteracion Priority2 (%d) %d\n", id, i);
        currentThread->Yield();
    }
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
    // for (unsigned i = 2; i <= 5; i++) {
    //     char *name = new char [16];
    //     sprintf(name, "%u", i);
    //     Thread *t = new Thread(name);
    //     t->Fork(SimpleThread, (void *) name);
    // }

    // SimpleThread((void *) "1");

    Thread *t1 = new Thread("t1", false, 9);
    Thread *t2 = new Thread("t2", false, 1);
    Thread *t3 = new Thread("t3", false, 1);

    int t2id = 1, t3id = 2;

    t1->Fork(Priority1, nullptr);
    t2->Fork(Priority2, &t2id);
    t3->Fork(Priority2, &t3id);
}
