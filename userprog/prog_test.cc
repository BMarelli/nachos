/// Test routines for demonstrating that Nachos can load a user program and
/// execute it.
///
/// Also, routines for testing the Console hardware device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include <stdio.h>

#include "address_space.hh"
#include "machine/console.hh"
#include "threads/semaphore.hh"
#include "threads/system.hh"

/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void StartProcess(const char *filename) {
    ASSERT(filename != nullptr);

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) {
        printf("Error: file `%s` not found.\n", filename);

        return;
    }

    AddressSpace *space = new AddressSpace(executable);
    currentThread->space = space;

    delete executable;

    space->InitRegisters();
    space->RestoreState();

    machine->Run();
    ASSERT(false);  // machine->Run() never returns
}

/// Data structures needed for the console test.
///
/// Threads making I/O requests wait on a `Semaphore` to delay until the I/O
/// completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

/// Console interrupt handlers.
///
/// Wake up the thread that requested the I/O.

static void ReadAvail(void *arg) { readAvail->V(); }

static void WriteDone(void *arg) { writeDone->V(); }

/// Test the console by echoing characters typed at the input onto the
/// output.
///
/// Stop when the user types a `q`.
void ConsoleTest(const char *in, const char *out) {
    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore(0);
    writeDone = new Semaphore(0);

    for (;;) {
        readAvail->P();  // Wait for character to arrive.
        char ch = console->GetChar();
        console->PutChar(ch);  // Echo it!
        writeDone->P();        // Wait for write to finish.
        if (ch == 'q') {
            return;  // If `q`, then quit.
        }
    }
}
