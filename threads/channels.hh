#ifndef NACHOS_CHANNELS__HH
#define NACHOS_CHANNELS__HH

#include "lock.hh"

class Channels {
   public:
    Channels();

    ~Channels();

    void Send(int message);

    void Receive(int *message);

   private:
    int buffer;

    Lock *sendLock, *receiveLock;

    Semaphore *sendSemaphore, *receiveSemaphore;
};
#endif
