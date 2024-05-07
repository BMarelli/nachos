#include "thread_test_join.hh"

#include <stdio.h>
#include <string.h>

#include "system.hh"

void ThreadJoin(void *name_) {
    char *name = (char *)name_;

    for (unsigned num = 0; num < 20; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
}

void ThreadTestJoin() {
    Thread *newThread = new Thread("1", true);
    newThread->Fork(ThreadJoin, (void *)"simple");
    newThread->Join();
    printf("!!! Thread joined\n");
}