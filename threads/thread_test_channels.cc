#include "thread_test_channels.hh"

#include <stdio.h>

#include "channel.hh"
#include "system.hh"

Channel *channel = new Channel();

#define YIELD currentThread->Yield()

bool senderDone, receiverDone;

void SenderThread(void *) {
    printf("Sending 0\n");
    channel->Send(0);
    printf("Sent 0\n");

    printf("Sending 1\n");
    channel->Send(1);
    printf("Sent 1\n");

    printf("Sending 2\n");
    channel->Send(2);
    printf("Sent 2\n");

    YIELD;

    printf("Sending 3\n");
    channel->Send(3);
    printf("Sent 3\n");

    senderDone = true;
}

void ReceiverThread(void *) {
    int response;

    printf("Receiving message\n");
    channel->Receive(&response);
    printf("Received %d\n", response);

    printf("Receiving message\n");
    channel->Receive(&response);
    printf("Received %d\n", response);

    YIELD;

    printf("Receiving message\n");
    channel->Receive(&response);
    printf("Received %d\n", response);

    printf("Receiving message\n");
    channel->Receive(&response);
    printf("Received %d\n", response);

    receiverDone = true;
}

void ThreadTestChannels() {
    Thread *sender = new Thread("Sender");
    Thread *receiver = new Thread("Receiver");

    sender->Fork(SenderThread, nullptr);
    receiver->Fork(ReceiverThread, nullptr);

    while (!senderDone || !receiverDone) {
        YIELD;
    }
}
