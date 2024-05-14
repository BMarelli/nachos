#include "channel.hh"

Channel::Channel() {
    sendLock = new Lock();
    receiveLock = new Lock();
    sendSemaphore = new Semaphore(0);
    receiveSemaphore = new Semaphore(0);
}

Channel::~Channel() {
    delete sendLock;
    delete receiveLock;
    delete sendSemaphore;
    delete receiveSemaphore;
}

void Channel::Send(int message) {
    sendLock->Acquire();
    buffer = message;
    receiveSemaphore->V();
    sendSemaphore->P();
    sendLock->Release();
}

void Channel::Receive(int *message) {
    receiveLock->Acquire();
    receiveSemaphore->P();
    *message = buffer;
    sendSemaphore->V();
    receiveLock->Release();
}
