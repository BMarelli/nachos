#include "thread_test_channels.hh"

#include <stdio.h>
#include <string.h>

#include "channels.hh"

static Channels *channel = new Channels("TestChannel");

void SenderThread(void *name_) {
    char *name = (char *)name_;
    int data = 42;
    printf("*** Thread `%s`: Sending data %d\n", name, data);
    channel->Send(data);
}

void ReceiverThread(void *name_) {
    char *name = (char *)name_;
    int response;
    channel->Receive(&response);
    printf("*** Thread `%s`: Received data %d\n", name, response);
}

void ThreadTestChannels() {
    Thread *senderThread = new Thread("Sender");
    Thread *receiverThread = new Thread("Receiver", true);

    senderThread->Fork(SenderThread, (void *)"Sender");
    receiverThread->Fork(ReceiverThread, (void *)"Receiver");
    receiverThread->Join();
    printf("!!! Threads have finished\n");
}