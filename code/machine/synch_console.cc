#include "synch_console.hh"

static void
SynchConsoleReadAvail(void *arg) {
    ASSERT(arg != nullptr);
    SynchConsole *synch_console = (SynchConsole *) arg;
    synch_console->ReadAvail();
}

static void
SynchConsoleWriteDone(void *arg) {
    ASSERT(arg != nullptr);
    SynchConsole *synch_console = (SynchConsole *) arg;
    synch_console->WriteDone();
}

SynchConsole::SynchConsole(const char *debugName)
{
    name = debugName;
    console = new Console(nullptr, nullptr, SynchConsoleReadAvail, SynchConsoleWriteDone, this);
    readAvail = new Semaphore("SynchConsole::readAvail", 0);
    writeDone = new Semaphore("SynchConsole::writeDone", 0);
    readLock = new Lock("SynchConsole::readLock");
    writeLock = new Lock("SynchConsole::writeLock");
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete readAvail;
    delete writeDone;
    delete readLock;
    delete writeLock;
}

char
SynchConsole::ReadChar() {
    readLock->Acquire();

    readAvail->P();
    char c = console->GetChar();

    readLock->Release();

    return c;
}

void
SynchConsole::WriteChar(char c) {
    writeLock->Acquire();

    console->PutChar(c);
    writeDone->P();

    writeLock->Release();
}

void
SynchConsole::ReadAvail() {
    readAvail->V();
}

void
SynchConsole::WriteDone() {
    writeDone->V();
}