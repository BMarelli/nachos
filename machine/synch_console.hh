#ifndef NACHOS_MACHINE_SYNCH_CONSOLE__HH
#define NACHOS_MACHINE_SYNCH_CONSOLE__HH

#include "machine/console.hh"
#include "threads/lock.hh"
#include "threads/semaphore.hh"

class SynchConsole {
   public:
    SynchConsole(const char *readFile, const char *writeFile);
    ~SynchConsole();

    int Read(char *data, int size);

    void Write(const char *data, int size);

   private:
    Console *console;
    Semaphore *readAvail, *writeDone;
    Lock *readLock, *writeLock;

    static void ReadAvail(void *arg);
    static void WriteDone(void *arg);
};

#endif
