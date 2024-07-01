#ifndef NACHOS_FILESYS_OPENFILEMANAGER__HH

#include <map>

#include "threads/rwlock.hh"

class OpenFileManager {
   public:
    OpenFileManager() = default;
    ~OpenFileManager();

    bool IsManaged(unsigned sector);
    void Manage(unsigned sector, unsigned referenceCount, RWLock *rwLock);
    void Unmanage(unsigned sector);

    unsigned GetReferenceCount(unsigned sector);
    unsigned IncrementReferenceCount(unsigned sector);
    unsigned DecrementReferenceCount(unsigned sector);

    RWLock *GetRWLock(unsigned sector);

   private:
    struct OpenFileInfo {
        unsigned referenceCount;
        RWLock *rwLock;
    };

    std::map<unsigned, OpenFileInfo> openFiles;

    OpenFileInfo GetOrCreateOpenFileInfo(unsigned sector);
};

#endif
