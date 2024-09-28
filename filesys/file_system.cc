/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "file_system.hh"

#include <stdio.h>

#include "directory.hh"
#include "file_header.hh"
#include "filesys/directory_entry.hh"
#include "lib/assert.hh"
#include "lib/bitmap.hh"
#include "lib/debug.hh"
#include "system.hh"

/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

inline OpenFile *openFreeMapFile() {
    FileHeader *fileHeader = new FileHeader;

    return new OpenFile(FREE_MAP_SECTOR, fileHeader);
}

inline OpenFile *openRootDirectoryFile() {
    FileHeader *fileHeader = new FileHeader;

    return new OpenFile(DIRECTORY_SECTOR, fileHeader);
}

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format) {
    DEBUG('f', "Initializing the file system.\n");

    freeMapFile = openFreeMapFile();
    rootDirectoryFile = openRootDirectoryFile();

    freeMap = new Bitmap(NUM_SECTORS);
    lock = new Lock();

    fileManager = new FileManager(freeMapFile, freeMap, lock);

    if (format) {
        DEBUG('f', "Formatting the file system.\n");

        Directory *dir = new Directory();

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(freeMapFile->GetFileHeader()->Allocate(freeMap, FREE_MAP_FILE_SIZE));

        freeMapFile->GetFileHeader()->WriteBack(FREE_MAP_SECTOR);
        rootDirectoryFile->GetFileHeader()->WriteBack(DIRECTORY_SECTOR);

        // Write the initial version of each file back to disk.
        // The directory at this point is completely empty; but the bitmap
        // has been changed to reflect the fact that sectors on the disk
        // have been allocated for the file headers and to hold the file
        // data for the directory and bitmap.
        freeMap->WriteBack(freeMapFile);
        dir->WriteBack(rootDirectoryFile);

        delete dir;
    } else {
        freeMapFile->GetFileHeader()->FetchFrom(FREE_MAP_SECTOR);
        rootDirectoryFile->GetFileHeader()->FetchFrom(DIRECTORY_SECTOR);

        freeMap->FetchFrom(freeMapFile);

        if (FileSystem::RemoveMarkedForDeletion(rootDirectoryFile)) {
            freeMap->WriteBack(freeMapFile);
        }
    }
}

FileSystem::~FileSystem() {
    delete freeMap;

    delete freeMapFile->GetFileHeader();
    delete freeMapFile;

    delete rootDirectoryFile->GetFileHeader();
    delete rootDirectoryFile;

    delete lock;

    delete fileManager;
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// `Create` the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// * `filepath` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool FileSystem::CreateFile(const char *filepath, unsigned initialSize) {
    ASSERT(filepath != nullptr);
    ASSERT(initialSize < MAX_FILE_SIZE);

    lock->Acquire();

    DEBUG('f', "Creating file %s, size %u\n", filepath, initialSize);

    OpenFile *directoryFile = LoadDirectory(filepath, false);
    if (directoryFile == nullptr) {
        lock->Release();

        return false;
    }

    filepath += FindLast(filepath, '/') + 1;

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    bool success;

    if (dir->HasEntry(filepath)) {
        success = false;  // Entry with given name is already in directory.
    } else {
        int sector = freeMap->Find();
        // Find a sector to hold the file header.
        if (sector == -1) {
            success = false;  // No free block for file header.
        } else if (!dir->Add(filepath, sector, false)) {
            success = false;  // No space in directory.
        } else {
            FileHeader *h = new FileHeader;
            success = h->Allocate(freeMap, initialSize);
            // Fails if no space on disk for data.
            if (success) {
                // Everything worked, flush all changes back to disk.
                h->WriteBack(sector);
                dir->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete h;
        }
    }

    if (!success) freeMap->FetchFrom(freeMapFile);

    delete dir;

    lock->Release();

    return success;
}

/// Open a file for reading and writing.
///
/// * `name` is the text name of the file to be opened.
OpenFile *FileSystem::Open(const char *filepath) {
    ASSERT(filepath != nullptr);

    lock->Acquire();

    DEBUG('f', "Opening file %s\n", filepath);

    OpenFile *directoryFile = LoadDirectory(filepath, false);
    if (directoryFile == nullptr) {
        lock->Release();

        return nullptr;
    }

    filepath += FindLast(filepath, '/') + 1;

    OpenFile *file = fileManager->Open(filepath, directoryFile);
    if (file == nullptr) {
        file = fileManager->Open(filepath, rootDirectoryFile);
    }

    lock->Release();

    return file;
}

/// Close a file.
///
/// * `file` is the file to be closed.
void FileSystem::Close(OpenFile *file) {
    ASSERT(file != nullptr);

    lock->Acquire();

    fileManager->Close(file);

    lock->Release();
}

/// Delete a file from the file system.
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool FileSystem::RemoveFile(const char *filepath) {
    ASSERT(filepath != nullptr);

    lock->Acquire();

    OpenFile *directoryFile = LoadDirectory(filepath, false);
    if (directoryFile == nullptr) {
        lock->Release();

        return false;
    }

    filepath += FindLast(filepath, '/') + 1;

    bool success = fileManager->Remove(filepath, directoryFile);

    lock->Release();

    return success;
}

/// Extend a file by a number of bytes.
///
/// * `file` is the file to be extended.
/// * `bytes` is the number of bytes to extend the file by.
bool FileSystem::ExtendFile(OpenFile *file, unsigned bytes) {
    bool hadLock = lock->IsHeldByCurrentThread();
    if (!hadLock) lock->Acquire();

    bool success = file->GetFileHeader()->Extend(freeMap, bytes);

    if (success) {
        file->GetFileHeader()->WriteBack(file->GetSector());
        freeMap->WriteBack(freeMapFile);
    }

    if (!hadLock) lock->Release();

    return success;
}

/// List all the files in the file system directory.
void FileSystem::List() {
    Directory *dir = new Directory();

    dir->FetchFrom(rootDirectoryFile);
    dir->List();

    delete dir;
}

static bool CheckForError(bool value, const char *message) {
    if (!value) DEBUG('f', "Error: %s\n", message);

    return !value;
}

static bool CheckFileSectors(unsigned sector, const FileHeader *fileHeader, Bitmap *shadowMap) {
    ASSERT(fileHeader != nullptr);

    bool error = false;

    error |= CheckForError(!shadowMap->Test(sector), "file header sector already marked");
    shadowMap->Mark(sector);

    for (unsigned i = 0; i < fileHeader->GetRaw()->numSectors; i++) {
        error |= CheckForError(!shadowMap->Test(fileHeader->GetSector(i)), "sector already marked");

        shadowMap->Mark(fileHeader->GetSector(i));
    }

    return error;
}

static bool CheckDirectory(const char *path, OpenFile *directoryFile, Bitmap *shadowMap) {
    bool error = false;

    error |= CheckFileSectors(directoryFile->GetSector(), directoryFile->GetFileHeader(), shadowMap);

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    const char *names[dir->GetRaw()->tableSize];

    bool repeated = false;
    for (unsigned i = 0; i < dir->GetRaw()->tableSize; i++) {
        if (!dir->GetRaw()->table[i].inUse) continue;

        DirectoryEntry *directoryEntry = &dir->GetRaw()->table[i];

        FileHeader *fileHeader = new FileHeader;
        fileHeader->FetchFrom(directoryEntry->sector);

        if (!directoryEntry->markedForDeletion) {
            for (unsigned j = 0; j < i; j++) {
                if (strcmp(names[j], directoryEntry->name) != 0) continue;

                DEBUG('f', "Found repeated names in directory %s: existing %s, new %s\n", path, names[j], directoryEntry->name);
                repeated = true;
            }

            names[i] = directoryEntry->name;
        }

        if (directoryEntry->isDirectory) {
            OpenFile *subDirFile = new OpenFile(directoryEntry->sector, fileHeader);

            Directory *subDir = new Directory();
            subDir->FetchFrom(subDirFile);

            if (directoryEntry->markedForDeletion && !subDir->IsEmpty()) {
                DEBUG('f', "Found directory entry marked for deletion but not empty at %s/%s\n", path, directoryEntry->name);
                error = true;
            }

            char *subPath = new char[strlen(path) + strlen(directoryEntry->name) + 2];
            strcpy(subPath, path);
            strcat(subPath, "/");
            strcat(subPath, directoryEntry->name);

            error |= CheckDirectory(subPath, subDirFile, shadowMap);

            delete[] subPath;
            delete subDir;
            delete subDirFile;
        } else {
            error |= CheckFileSectors(directoryEntry->sector, fileHeader, shadowMap);
        }

        delete fileHeader;
    }

    error |= CheckForError(!repeated, "found duplicate names in directory");

    delete dir;

    return error;
}

static bool CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap) {
    bool error = false;

    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        if (shadowMap->Test(i) == freeMap->Test(i)) continue;

        DEBUG('f', "FreeMap sector %u: expected %u, actual %u.\n", i, shadowMap->Test(i), freeMap->Test(i));
        error = true;
    }

    return error;
}

bool FileSystem::Check() {
    DEBUG('f', "Performing filesystem check...\n");

    bool error = false;

    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);

    error |= CheckFileSectors(freeMapFile->GetSector(), freeMapFile->GetFileHeader(), shadowMap);
    error |= CheckDirectory("/", rootDirectoryFile, shadowMap);

    error |= CheckForError(!CheckBitmaps(freeMap, shadowMap), "inconsistent freemap");

    delete shadowMap;

    DEBUG('f', error ? "Filesystem check failed.\n" : "Filesystem check succeeded.\n");

    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void FileSystem::Print() {
    printf("--------------------------------\n");

    freeMapFile->GetFileHeader()->Print("Bitmap");

    printf("--------------------------------\n");

    rootDirectoryFile->GetFileHeader()->Print("Directory");

    printf("--------------------------------\n");

    freeMap->Print();

    printf("--------------------------------\n");

    Directory *dir = new Directory();

    dir->FetchFrom(rootDirectoryFile);
    dir->Print();

    delete dir;

    printf("--------------------------------\n");
}

/// Create a directory.
///
/// * `path` is the path of the directory to be created.
bool FileSystem::CreateDirectory(const char *path) {
    ASSERT(path != nullptr);

    lock->Acquire();

    OpenFile *directoryFile = LoadDirectory(path, false);
    if (directoryFile == nullptr) {
        lock->Release();

        return false;
    }

    path += FindLast(path, '/') + 1;

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    bool success;

    if (dir->HasEntry(path)) {
        success = false;  // Entry with given name is already in directory.
    } else {
        int sector = freeMap->Find();
        // Find a sector to hold the file header.
        if (sector == -1) {
            success = false;  // No free block for file header.
        } else {
            success = dir->Add(path, sector, true);
            if (success) {
                FileHeader *subDirectoryFileHeader = new FileHeader;
                OpenFile *subDirectoryFile = new OpenFile(sector, subDirectoryFileHeader);
                Directory *subDirectory = new Directory();

                // Everything worked, flush all changes back to disk.
                subDirectoryFileHeader->WriteBack(sector);
                subDirectory->WriteBack(subDirectoryFile);
                dir->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);

                delete subDirectoryFileHeader;
                delete subDirectoryFile;
                delete subDirectory;
            }
        }
    }

    if (!success) freeMap->FetchFrom(freeMapFile);

    lock->Release();

    return success;
}

/// Change the current directory.
///
/// * `path` is the path of the directory to be changed to.
bool FileSystem::ChangeDirectory(const char *path) {
    lock->Acquire();

    OpenFile *targetDirectoryFile;
    if (path == nullptr) {
        targetDirectoryFile = rootDirectoryFile;
    } else {
        targetDirectoryFile = LoadDirectory(path, true);
        if (targetDirectoryFile == nullptr) {
            lock->Release();

            return false;
        }
    }

    OpenFile *directoryFile = GetCurrentWorkingDirectory();
    if (directoryFile != rootDirectoryFile) {
        delete directoryFile->GetFileHeader();
        delete directoryFile;
    }

    if (targetDirectoryFile == rootDirectoryFile) {
        currentThread->SetCurrentWorkingDirectory(nullptr);
    } else {
        currentThread->SetCurrentWorkingDirectory(targetDirectoryFile);
    }

    lock->Release();

    return true;
}

/// List the contents of a directory.
///
/// * `path` is the path of the directory to be listed.
char *FileSystem::ListDirectoryContents(const char *path) {
    lock->Acquire();

    OpenFile *directoryFile;
    if (path == nullptr) {
        directoryFile = GetCurrentWorkingDirectory();
    } else {
        directoryFile = LoadDirectory(path, true);
        if (directoryFile == nullptr) {
            lock->Release();

            return nullptr;
        }
    }

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    char *contents = dir->ListContents();

    delete dir;

    lock->Release();

    return contents;
}

/// Remove a directory.
///
/// * `path` is the name of the directory to be removed.
bool FileSystem::RemoveDirectory(const char *path) {
    ASSERT(path != nullptr);

    lock->Acquire();

    OpenFile *directoryFile = LoadDirectory(path, false);
    if (directoryFile == nullptr) {
        lock->Release();

        return false;
    }

    path += FindLast(path, '/') + 1;

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    int sector = dir->FindDirectory(path);
    if (sector == -1) {
        delete dir;

        lock->Release();

        return false;  // directory not found
    }

    FileHeader *dirToRemoveFileHeader = new FileHeader;
    dirToRemoveFileHeader->FetchFrom(sector);

    OpenFile *dirToRemoveFile = new OpenFile(sector, dirToRemoveFileHeader);

    Directory *dirToRemove = new Directory();
    dirToRemove->FetchFrom(dirToRemoveFile);

    if (!dirToRemove->IsEmpty()) {
        delete dir;
        delete dirToRemove;
        delete dirToRemoveFile;
        delete dirToRemoveFileHeader;

        lock->Release();

        return false;  // directory not empty
    }

    // remove data blocks
    dirToRemoveFile->GetFileHeader()->Deallocate(freeMap);

    // remove header block
    freeMap->Clear(sector);
    freeMap->WriteBack(freeMapFile);

    // remove from directory
    dir->Remove(path);
    dir->WriteBack(directoryFile);

    delete dir;
    delete dirToRemove;
    delete dirToRemoveFile;
    delete dirToRemoveFileHeader;

    lock->Release();

    return true;
}

bool FileSystem::RemoveMarkedForDeletion(OpenFile *directoryFile) {
    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    bool dirty = false;

    for (unsigned i = 0; i < dir->GetRaw()->tableSize; i++) {
        if (!dir->GetRaw()->table[i].inUse) continue;

        unsigned sector = dir->GetRaw()->table[i].sector;

        if (dir->GetRaw()->table[i].isDirectory) {
            FileHeader *subDirFileHeader = new FileHeader;
            subDirFileHeader->FetchFrom(sector);

            OpenFile *subDirFile = new OpenFile(sector, subDirFileHeader);

            dirty |= RemoveMarkedForDeletion(subDirFile);

            delete subDirFile;
            delete subDirFileHeader;
        }

        if (dir->IsMarkedForDeletion(sector)) {
            DEBUG('f', "Removing file marked for deletion at sector %u\n", sector);

            FileHeader *fileHeader = new FileHeader;
            fileHeader->FetchFrom(sector);

            // remove data blocks
            fileHeader->Deallocate(freeMap);

            // remove header block
            freeMap->Clear(sector);

            // remove from directory
            dir->RemoveMarkedForDeletion(sector);

            delete fileHeader;

            dirty = true;
        }
    }

    if (dirty) dir->WriteBack(directoryFile);

    delete dir;

    return dirty;
}

OpenFile *FileSystem::LoadDirectory(const char *path, bool includeLastToken) {
    ASSERT(lock->IsHeldByCurrentThread());
    ASSERT(path != nullptr);

    OpenFile *directoryFile = GetCurrentWorkingDirectory();

    Directory *dir = new Directory();
    dir->FetchFrom(directoryFile);

    char *buffer = CopyString(path);
    char *originalBuffer = buffer;

    if (!includeLastToken) {
        int lastSlash = FindLast(buffer, '/');
        if (lastSlash == -1) return directoryFile;

        buffer[lastSlash] = '\0';
    }

    char *token = strtok_r(buffer, "/", &buffer);

    if (token == nullptr) {
        delete dir;
        delete[] originalBuffer;

        return directoryFile;
    }

    int sector = dir->FindDirectory(token);
    if (sector == -1) {
        delete dir;
        delete[] originalBuffer;

        return nullptr;
    }

    directoryFile = new OpenFile(sector, new FileHeader);
    directoryFile->GetFileHeader()->FetchFrom(sector);
    dir->FetchFrom(directoryFile);

    token = strtok_r(nullptr, "/", &buffer);

    while (token != nullptr) {
        sector = dir->FindDirectory(token);
        if (sector == -1) {
            delete directoryFile->GetFileHeader();
            delete directoryFile;

            delete dir;
            delete[] originalBuffer;

            return nullptr;
        }

        OpenFile *subDirectoryFile = new OpenFile(sector, new FileHeader);
        subDirectoryFile->GetFileHeader()->FetchFrom(sector);
        dir->FetchFrom(subDirectoryFile);

        delete directoryFile->GetFileHeader();
        delete directoryFile;

        directoryFile = subDirectoryFile;

        token = strtok_r(nullptr, "/", &buffer);
    }

    delete dir;

    delete[] originalBuffer;

    return directoryFile;
}

OpenFile *FileSystem::GetCurrentWorkingDirectory() {
    OpenFile *cwd = currentThread->GetCurrentWorkingDirectory();
    if (cwd == nullptr) {
        return rootDirectoryFile;
    }

    cwd->GetFileHeader()->FetchFrom(cwd->GetSector());

    return cwd;
}
