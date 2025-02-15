/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "lock.hh"

#include "system.hh"

Lock::Lock() {
    semaphore = new Semaphore(1);
    thread = nullptr;
}

Lock::~Lock() { delete semaphore; }

void Lock::Acquire() {
    ASSERT(!IsHeldByCurrentThread());

    // If a thread with lower priority than the current thread is holding the
    // lock with a lower priority, the prioritize it.
    if (thread && thread->GetPriority() < currentThread->GetPriority()) {
        scheduler->Prioritize(thread);
    }

    semaphore->P();
    thread = currentThread;
}

void Lock::Release() {
    ASSERT(IsHeldByCurrentThread());

    // If the thread has been prioritized, restore its original priority.
    if (thread->GetPriority() > thread->GetOriginalPriority()) {
        scheduler->RestoreOriginalPriority(thread);
    }

    thread = nullptr;
    semaphore->V();
}

bool Lock::IsHeldByCurrentThread() const { return thread == currentThread; }
