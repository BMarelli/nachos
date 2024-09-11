#include "rwlock.hh"

#include "lib/assert.hh"
#include "threads/system.hh"

RWLock::RWLock() {
    lock = new Lock();
    condition = new Condition(lock);
    activeReaders = 0;
    waitingWriters = 0;
    activeWriter = nullptr;
}

RWLock::~RWLock() {
    delete lock;
    delete condition;
}

void RWLock::AcquireRead() {
    if (currentThread == activeWriter) return;

    lock->Acquire();

    while (waitingWriters > 0 || activeWriter != nullptr) condition->Wait();

    ASSERT(activeWriter == nullptr);
    activeReaders++;

    lock->Release();
}

void RWLock::ReleaseRead() {
    if (currentThread == activeWriter) return;

    lock->Acquire();

    ASSERT(activeWriter == nullptr && activeReaders > 0);
    activeReaders--;

    if (activeReaders == 0) condition->Broadcast();

    lock->Release();
}

void RWLock::AcquireWrite() {
    lock->Acquire();

    waitingWriters++;

    while (activeReaders > 0 || activeWriter != nullptr) condition->Wait();

    ASSERT(activeReaders == 0 && activeWriter == nullptr);

    waitingWriters--;
    activeWriter = currentThread;

    lock->Release();
}

void RWLock::ReleaseWrite() {
    lock->Acquire();

    ASSERT(activeReaders == 0 && currentThread == activeWriter);

    activeWriter = nullptr;

    condition->Broadcast();

    lock->Release();
}
