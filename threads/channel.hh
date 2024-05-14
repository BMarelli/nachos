#ifndef NACHOS_CHANNEL__HH
#define NACHOS_CHANNEL__HH

#include "lock.hh"

class Channel {
   public:
    Channel();

    ~Channel();

    void Send(int message);

    void Receive(int *message);

   private:
    int buffer;

    Lock *sendLock, *receiveLock;

    Semaphore *sendSemaphore, *receiveSemaphore;
};
#endif
