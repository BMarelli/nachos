#include "channels.hh"

Channels::Channels() {
    sendLock = new Lock();
    receiveLock = new Lock();
    sendSemaphore = new Semaphore(0);
    receiveSemaphore = new Semaphore(0);
}

Channels::~Channels() {
    delete sendLock;
    delete receiveLock;
    delete sendSemaphore;
    delete receiveSemaphore;
}

void Channels::Send(int message) {
    sendLock->Acquire();
    buffer = message;
    receiveSemaphore->V();
    sendSemaphore->P();
    sendLock->Release();
}

void Channels::Receive(int *message) {
    receiveLock->Acquire();
    receiveSemaphore->P();
    *message = buffer;
    sendSemaphore->V();
    receiveLock->Release();
}
