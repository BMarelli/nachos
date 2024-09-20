#ifndef NACHOS_FILESYS_FILEMANAGER__HH
#define NACHOS_FILESYS_FILEMANAGER__HH

#include <map>

#include "filesys/file_header.hh"
#include "filesys/open_file.hh"
#include "threads/rwlock.hh"

class FileManager {
   public:
    FileManager(OpenFile *freeMapFile, Bitmap *freeMap, Lock *lock);
    ~FileManager();

    OpenFile *Open(const char *name, OpenFile *directoryFile);
    void Close(OpenFile *file);
    bool Remove(const char *name, OpenFile *directoryFile);

   private:
    OpenFile *freeMapFile;
    Bitmap *freeMap;
    Lock *lock;

    struct OpenFileInfo {
        unsigned directorySector;

        unsigned referenceCount;
        RWLock *rwLock;
        FileHeader *fileHeader;
    };

    std::map<unsigned, OpenFileInfo> openFiles;

    bool IsManaged(unsigned sector);
};

#endif
