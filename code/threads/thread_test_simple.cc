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

#ifdef CHANNEL_TEST
#include "channel.hh"
Channel *channel = new Channel("test channel");
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

#ifdef CHANNEL_TEST
void channel_thread_1(void* arg) {
    int number = 10;

    DEBUG('c', "thread_1 quiere enviar %d\n", number);
    channel->Send(number);
    DEBUG('c', "thread_1 consiguio enviar el dato\n");

    sleep(5);

    DEBUG('c', "thread_1 quiere recibir un dato\n");
    channel->Receive(&number);
    DEBUG('c', "thread_1 consiguio recibir el dato: %d\n", number);

}

void channel_thread_2(void* arg) {
    int number;

    sleep(5);

    DEBUG('c', "thread_2 quiere recibir un dato\n");
    channel->Receive(&number);
    DEBUG('c', "thread_2 consiguio recibir el dato: %d\n", number);

    number = 5;
    DEBUG('c', "thread_2 quiere enviar %d\n", number);
    channel->Send(number);
    DEBUG('c', "thread_2 consiguio enviar el dato\n");
}
#endif

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
    //     Thread *t = new Thread(name, false, PRIORITY_DEFAULT);
    //     t->Fork(SimpleThread, (void *) name);
    // }

    // SimpleThread((void *) "1");

#ifdef CHANNEL_TEST
    DEBUG('c', "CHANNEL TEST\n");
    Thread *t1 = new Thread("t1", false, PRIORITY_DEFAULT);
    Thread *t2 = new Thread("t2", false, PRIORITY_DEFAULT);

    t1->Fork(channel_thread_1, nullptr);
    t2->Fork(channel_thread_2, nullptr);
#endif
}
