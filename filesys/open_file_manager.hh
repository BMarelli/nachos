#ifndef NACHOS_FILESYS_OPENFILEMANAGER__HH
#define NACHOS_FILESYS_OPENFILEMANAGER__HH

#include <map>

#include "filesys/file_header.hh"
#include "threads/rwlock.hh"

class OpenFileManager {
   public:
    OpenFileManager() = default;
    ~OpenFileManager();

    bool IsManaged(unsigned sector);
    void Manage(unsigned sector, unsigned referenceCount, RWLock *rwLock, FileHeader *fileHeader);
    void Unmanage(unsigned sector);

    unsigned GetReferenceCount(unsigned sector);
    unsigned IncrementReferenceCount(unsigned sector);
    unsigned DecrementReferenceCount(unsigned sector);

    RWLock *GetRWLock(unsigned sector);

    FileHeader *GetFileHeader(unsigned sector);

   private:
    struct OpenFileInfo {
        unsigned referenceCount;
        RWLock *rwLock;
        FileHeader *fileHeader;
    };

    std::map<unsigned, OpenFileInfo> openFiles;

    OpenFileInfo GetOrCreateOpenFileInfo(unsigned sector);
};

#endif
