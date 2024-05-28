#ifndef NACHOS_CHANNEL__HH
#define NACHOS_CHANNEL__HH

#include "threads/lock.hh"
#include "threads/semaphore.hh"

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
