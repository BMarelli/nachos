#ifndef NACHOS_FILESYS_SYNCH_OPEN_FILE__HH
#define NACHOS_FILESYS_SYNCH_OPEN_FILE__HH

#include "open_file.hh"

class RWLock;

class SynchOpenFile : public OpenFile {
   public:
    SynchOpenFile(int _sector, FileHeader *_hdr, RWLock *_rwLock) : OpenFile(_sector, _hdr), rwLock(_rwLock) {}

    int ReadAt(char *into, unsigned numBytes, unsigned position) override;

    int WriteAt(const char *from, unsigned numBytes, unsigned position) override;

   private:
    RWLock *rwLock;
};

#endif
