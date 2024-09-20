#include "file_manager.hh"

#include "filesys/directory.hh"
#include "filesys/file_system.hh"
#include "filesys/synch_open_file.hh"
#include "lib/assert.hh"
#include "lib/debug.hh"

FileManager::FileManager(OpenFile *_freeMapFile, Bitmap *_freeMap, Lock *_lock)
    : freeMapFile(_freeMapFile), freeMap(_freeMap), lock(_lock) {
    ASSERT(freeMapFile != nullptr);
    ASSERT(freeMap != nullptr);
    ASSERT(lock != nullptr);
}

FileManager::~FileManager() {
    for (auto &openFile : openFiles) {
        delete openFile.second.rwLock;
        delete openFile.second.fileHeader;
    }
}

bool FileManager::IsManaged(unsigned sector) { return openFiles.find(sector) != openFiles.end(); }

OpenFile *FileManager::Open(const char *name, OpenFile *directoryFile) {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(name != nullptr);
    ASSERT(directoryFile != nullptr);

    DEBUG('f', "[FileManager] opening file %s\n", name);

    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(directoryFile);

    int sector = dir->Find(name);
    if (sector == -1) {
        delete dir;

        return nullptr;  // file not found
    }

    delete dir;

    DEBUG('f', "[FileManager] file found at sector %u\n", sector);

    if (!IsManaged(sector)) {
        DEBUG('f', "[FileManager] file is not managed, creating new entry\n");

        RWLock *rwLock = new RWLock();
        FileHeader *fileHeader = new FileHeader;
        fileHeader->FetchFrom(sector);

        openFiles[sector] = {directoryFile->GetSector(), 0, rwLock, fileHeader};
    } else {
        DEBUG('f', "[FileManager] file is already managed\n");

        ASSERT(openFiles[sector].directorySector == directoryFile->GetSector());
    }

    openFiles[sector].referenceCount++;

    DEBUG('f', "[FileManager] file opened, reference count: %u\n", openFiles[sector].referenceCount);

    return new SynchOpenFile(sector, openFiles[sector].fileHeader, openFiles[sector].rwLock);
}

void FileManager::Close(OpenFile *file) {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(file != nullptr);

    unsigned sector = file->GetSector();

    ASSERT(sector < NUM_SECTORS);
    ASSERT(IsManaged(sector));

    DEBUG('f', "[FileManager] closing file at sector %u\n", sector);

    openFiles[sector].referenceCount--;

    if (openFiles[sector].referenceCount == 0) {
        DEBUG('f', "[FileManager] file is no longer referenced, cleaning up entry\n");

        FileHeader *directoryFileHeader = new FileHeader;
        directoryFileHeader->FetchFrom(openFiles[sector].directorySector);

        OpenFile *directoryFile = new OpenFile(openFiles[sector].directorySector, directoryFileHeader);

        Directory *dir = new Directory(NUM_DIR_ENTRIES);
        dir->FetchFrom(directoryFile);

        if (dir->IsMarkedForDeletion(sector)) {
            DEBUG('f', "[FileManager] file is marked for deletion\n");

            // remove data blocks
            file->GetFileHeader()->Deallocate(freeMap);

            // remove header block
            freeMap->Clear(sector);
            freeMap->WriteBack(freeMapFile);

            // remove from directory
            dir->RemoveMarkedForDeletion(sector);
            dir->WriteBack(directoryFile);
        }

        delete dir;

        delete directoryFile;

        delete directoryFileHeader;

        delete openFiles[sector].rwLock;
        delete openFiles[sector].fileHeader;

        openFiles.erase(sector);
    }

    delete file;
}

bool FileManager::Remove(const char *name, OpenFile *directoryFile) {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(name != nullptr);
    ASSERT(directoryFile != nullptr);

    DEBUG('f', "[FileManager] removing file %s\n", name);

    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(directoryFile);

    int sector = dir->Find(name);
    if (sector == -1) {
        delete dir;

        return false;  // file not found
    }

    DEBUG('f', "[FileManager] file found at sector %u\n", sector);

    if (!IsManaged(sector)) {
        FileHeader *fileHeader = new FileHeader;
        fileHeader->FetchFrom(sector);

        // remove data blocks
        fileHeader->Deallocate(freeMap);

        // remove header block
        freeMap->Clear(sector);
        freeMap->WriteBack(freeMapFile);

        // remove from directory
        dir->Remove(name);
        dir->WriteBack(directoryFile);

        delete fileHeader;

        DEBUG('f', "[FileManager] file removed succesfully\n");

        delete dir;

        return true;
    }

    ASSERT(openFiles[sector].referenceCount > 0);
    ASSERT(openFiles[sector].directorySector == directoryFile->GetSector());

    DEBUG('f', "[FileManager] file is still being referenced, marking for deletion\n");

    dir->MarkForDeletion(sector);
    dir->WriteBack(directoryFile);

    delete dir;

    return true;
}
