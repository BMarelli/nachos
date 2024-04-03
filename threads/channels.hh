#ifndef NACHOS_CHANNELS__HH
#define NACHOS_CHANNELS__HH

#include "lock.hh"

class Channels {
   public:
    Channels(const char *name);

    ~Channels();

    void Send(int message);

    void Receive(int *message);

   private:
    const char *sendLockName, *receiveLockName, *sendSemaphoreName, *receiveSemaphoreName;

    int buffer;

    Lock *sendLock, *receiveLock;

    Semaphore *sendSemaphore, *receiveSemaphore;
};
#endif
