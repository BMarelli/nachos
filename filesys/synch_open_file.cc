#include "synch_open_file.hh"

#include "rwlock.hh"

int SynchOpenFile::ReadAt(char *into, unsigned numBytes, unsigned position) {
    rwLock->AcquireRead();

    int numRead = OpenFile::ReadAt(into, numBytes, position);

    rwLock->ReleaseRead();

    return numRead;
}

int SynchOpenFile::WriteAt(const char *from, unsigned numBytes, unsigned position) {
    rwLock->AcquireWrite();

    int numWritten = OpenFile::WriteAt(from, numBytes, position);

    rwLock->ReleaseWrite();

    return numWritten;
}
