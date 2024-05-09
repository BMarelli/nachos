#include "channels.hh"

#include "lib/utility.hh"

Channels::Channels(const char *name) {
    sendLockName = make_debug_name(name, "sendLock");
    receiveLockName = make_debug_name(name, "receiveLock");
    sendSemaphoreName = make_debug_name(name, "sendSemaphore");
    receiveSemaphoreName = make_debug_name(name, "receiveSemaphore");

    sendLock = new Lock(sendLockName);
    receiveLock = new Lock(receiveLockName);
    sendSemaphore = new Semaphore(sendSemaphoreName, 0);
    receiveSemaphore = new Semaphore(receiveSemaphoreName, 0);
}

Channels::~Channels() {
    delete sendLockName;
    delete receiveLockName;
    delete sendSemaphoreName;
    delete receiveSemaphoreName;

    delete sendLock;
    delete receiveLock;
    delete sendSemaphore;
    delete receiveSemaphore;
}

void Channels::Send(int message) {
    sendLock->Acquire();
    sendSemaphore->P();
    buffer = message;
    receiveSemaphore->V();
    sendLock->Release();
}

void Channels::Receive(int *message) {
    receiveLock->Acquire();
    sendSemaphore->V();
    receiveSemaphore->P();
    *message = buffer;
    receiveLock->Release();
}
