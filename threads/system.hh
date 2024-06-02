/// All global variables used in Nachos are defined here.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_SYSTEM__HH
#define NACHOS_THREADS_SYSTEM__HH

#include "machine/interrupt.hh"
#include "machine/statistics.hh"
#include "machine/system_dep.hh"  // IWYU pragma: export
#include "machine/timer.hh"
#include "scheduler.hh"
#include "thread.hh"

/// Initialization and cleanup routines.

// Initialization, called before anything else.
extern void Initialize(int argc, char **argv);

// Cleanup, called when Nachos is done.
extern void Cleanup();

extern Thread *currentThread;        ///< The thread holding the CPU.
extern Thread *threadToBeDestroyed;  ///< The thread that just finished.
extern Scheduler *scheduler;         ///< The ready list.
extern Interrupt *interrupt;         ///< Interrupt status.
extern Statistics *stats;            ///< Performance metrics.
extern Timer *timer;                 ///< The hardware alarm clock.

#ifdef USER_PROGRAM
#include "lib/table.hh"
#include "machine/machine.hh"
#include "synch_console.hh"
#include "userprog/core_map.hh"

extern Machine *machine;               // User program memory and registers.
extern CoreMap *memoryMap;             // Map of free memory frames.
extern SynchConsole *synchConsole;     // Synchronized console.
extern Table<Thread *> *processTable;  // Table of processes.

#if defined(USE_TLB) || defined(DEMAND_LOADING)
// When using TLB or demand loading, we need to retry the read/write as the first attempt may result in a page fault.
#define READ_MEM(addr, size, value) ASSERT_WITH_RETRY(machine->ReadMem(addr, size, value))
#define WRITE_MEM(addr, size, value) ASSERT_WITH_RETRY(machine->WriteMem(addr, size, value))
#else
// Otherwise, we don't need to retry the read/write as pages are always valid.
#define READ_MEM(addr, size, value) ASSERT(machine->ReadMem(addr, size, value))
#define WRITE_MEM(addr, size, value) ASSERT(machine->WriteMem(addr, size, value))
#endif

#endif

#ifdef FILESYS_NEEDED  // *FILESYS* or *FILESYS_STUB*.
#include "filesys/file_system.hh"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "filesys/synch_disk.hh"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "network/post.hh"
extern PostOffice *postOffice;
#endif

#endif
