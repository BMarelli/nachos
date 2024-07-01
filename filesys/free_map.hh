#ifndef NACHOS_FILESYS_FREE_MAP__HH

#include "filesys/directory.hh"
#include "filesys/open_file.hh"
#include "lib/bitmap.hh"
#include "lib/utility.hh"
#include "threads/lock.hh"
#include "threads/rwlock.hh"

/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files. These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

static const unsigned FREE_MAP_FILE_SIZE = NUM_SECTORS / BITS_IN_BYTE;

class FreeMap {
   public:
    FreeMap();
    ~FreeMap();

    void Format(Directory *directory);
    void Load();

    void Begin();
    void Commit();
    void Abort();

    unsigned Find();
    void Mark(unsigned which);
    bool Test(unsigned which) const;
    void Clear(unsigned which);
    unsigned CountClear() const;

    bool Allocate(FileHeader *fileHeader, unsigned size);
    void Deallocate(FileHeader *fileHeader);

    void Print() const;

   private:
    bool valid;
    bool dirty;

    Lock *lock;

    FileHeader *fileHeader;
    RWLock *rwLock;

    OpenFile *file;

    Bitmap *bitmap;
};

#endif
