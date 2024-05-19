#include "synch_console.hh"

#include <cstdio>

SynchConsole::SynchConsole(const char *readFile, const char *writeFile) {
    console = new Console(readFile, writeFile, ReadAvail, WriteDone, this);
    readAvail = new Semaphore(0);
    writeDone = new Semaphore(0);
    readLock = new Lock();
    writeLock = new Lock();
}

SynchConsole::~SynchConsole() {
    delete console;
    delete readAvail;
    delete writeDone;
    delete readLock;
    delete writeLock;
}

int SynchConsole::Read(char *data, int size) {
    readLock->Acquire();

    int i;
    for (i = 0; i < size; i++) {
        readAvail->P();
        data[i] = console->GetChar();

        if (data[i] == EOF) break;
    }

    readLock->Release();

    return i;
}

void SynchConsole::Write(const char *data, int size) {
    writeLock->Acquire();

    for (int i = 0; i < size; i++) {
        console->PutChar(data[i]);
        writeDone->P();
    }

    writeLock->Release();
}

void SynchConsole::ReadAvail(void *arg) {
    SynchConsole *synchConsole = (SynchConsole *)arg;
    synchConsole->readAvail->V();
}

void SynchConsole::WriteDone(void *arg) {
    SynchConsole *synchConsole = (SynchConsole *)arg;
    synchConsole->writeDone->V();
}
