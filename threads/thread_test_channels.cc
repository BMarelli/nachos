#include "thread_test_channels.hh"

#include <stdio.h>

#include <cstdlib>

#include "channel.hh"
#include "system.hh"

Channel *channel = new Channel();

const int NUM_SENDERS = 6;
const int NUM_RECEIVERS = 4;

void randomYield() {
    if (rand() % 2 == 0) {
        currentThread->Yield();
    }
}

void SenderThread(void *n) {
    for (int i = 0; i < NUM_RECEIVERS; ++i) {
        printf("Sender %ld: waiting to send message %d\n", (long)n, i);
        channel->Send(i);
        printf("Sender %ld: sent message %d\n", (long)n, i);

        randomYield();
    }
}

void ReceiverThread(void *n) {
    for (int i = 0; i < NUM_SENDERS; ++i) {
        int response;
        printf("Receiver %ld: waiting to receive message\n", (long)n);
        channel->Receive(&response);
        printf("Receiver %ld: received message %d\n", (long)n, response);

        randomYield();
    }
}

void ThreadTestChannels() {
    Thread *senders[NUM_SENDERS];
    Thread *receivers[NUM_RECEIVERS];

    for (long i = 0; i < NUM_SENDERS; ++i) {
        char *name = new char[16];
        sprintf(name, "Sender %ld", i);
        senders[i] = new Thread(name, true);

        senders[i]->Fork(SenderThread, (void *)i);
    }

    for (long i = 0; i < NUM_RECEIVERS; ++i) {
        char *name = new char[16];
        sprintf(name, "Receiver %ld", i);
        receivers[i] = new Thread(name, true);
        receivers[i]->Fork(ReceiverThread, (void *)i);
    }

    for (int i = 0; i < NUM_SENDERS; ++i) {
        senders[i]->Join();
    }

    for (int i = 0; i < NUM_RECEIVERS; ++i) {
        receivers[i]->Join();
    }

    printf("Test completed successfully!\n");
}
