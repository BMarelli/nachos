#ifndef NACHOS_FILESYS_RWLOCK__HH
#define NACHOS_FILESYS_RWLOCK__HH

#include "condition.hh"
#include "lock.hh"

class RWLock {
   public:
    RWLock();
    ~RWLock();

    /// Acquires a read lock.
    /// If the current thread is the active writer, it returns immediately.
    /// Otherwise, it waits until there are no waiting writers and no active writer.
    /// There can be multiple active readers.
    void AcquireRead();

    /// Releases a read lock.
    /// If the current thread is the active writer, it returns immediately as the thread still holds the write lock.
    /// Otherwise, it signals any waiting (writer) threads if there are no other active readers.
    void ReleaseRead();

    /// Acquires a write lock.
    /// Waits until there are no active readers and no active writer.
    /// There can be only one active writer.
    void AcquireWrite();

    /// Releases a write lock.
    /// Signals all waiting threads.
    void ReleaseWrite();

   private:
    Lock *lock;
    Condition *condition;
    unsigned activeReaders, waitingWriters;
    Thread *activeWriter;
};

#endif
