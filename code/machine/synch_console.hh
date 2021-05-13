#ifndef SYNCHCONSOLE_HH
#define SYNCHCONSOLE_HH

#include "machine/console.hh"
#include "threads/semaphore.hh"
#include "threads/lock.hh"

class SynchConsole {
public:
    SynchConsole(const char* name);

    ~SynchConsole();

    char ReadChar();
    void WriteChar(char c);

    void ReadAvail();
    void WriteDone();

private:
    const char* name;
    Console* console;
    Semaphore* readAvail;
    Semaphore* writeDone;
    Lock* readLock;
    Lock* writeLock;
};

#endif
