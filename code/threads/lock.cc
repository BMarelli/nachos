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
#include "./../lib/utility.hh"
#include "system.hh"


/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    name = debugName;
    semaphore = new Semaphore(name, 1);
    thread_lock = nullptr;
}

Lock::~Lock()
{
    delete semaphore;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(!IsHeldByCurrentThread());

    if (thread_lock && thread_lock->GetPriority() < currentThread->GetPriority()) {
        unsigned temp = thread_lock->GetPriority();
        thread_lock->SetPriority(currentThread->GetPriority());
        scheduler->UpdatePriority(thread_lock, temp);
    }

    semaphore->P();
    thread_lock = currentThread;
}

void
Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());

    currentThread->RestorePriority();
    thread_lock = nullptr;
    semaphore->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return thread_lock == currentThread;
}
