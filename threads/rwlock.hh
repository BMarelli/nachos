#ifndef NACHOS_FILESYS_RWLOCK__HH
#define NACHOS_FILESYS_RWLOCK__HH

#include "condition.hh"
#include "lock.hh"

class RWLock {
   public:
    RWLock();
    ~RWLock();

    void AcquireRead();
    void ReleaseRead();
    void AcquireWrite();
    void ReleaseWrite();

   private:
    Lock *lock;
    Condition *condition;
    unsigned activeReaders, waitingWriters;
    Thread *activeWriter;
};

#endif
