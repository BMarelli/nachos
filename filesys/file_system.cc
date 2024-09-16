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
/// directory and/or bitmap, if the operation succeeds, the Fs are
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
#include <string.h>
#include <unistd.h>

#include "file_header.hh"
#include "filesys/directory.hh"
#include "filesys/synch_open_file.hh"
#include "filesys/unique_open_file.hh"
#include "lib/bitmap.hh"
#include "lib/debug.hh"
#include "lib/utility.hh"
#include "system.hh"
#include "threads/rwlock.hh"

/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

// TODO: move somewher else
const char *GetFileName(const char *path) {
    ASSERT(path != nullptr);

    const char *name = strrchr(path, '/');
    if (name == nullptr) {
        return path;
    }

    return name + 1;
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

    lock = new Lock();
    openFileManager = new OpenFileManager();

    if (format) {
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        Directory *dir = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapH = new FileHeader;
        FileHeader *dirH = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapH->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        ASSERT(dirH->Allocate(freeMap, DIRECTORY_FILE_SIZE));

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapH->WriteBack(FREE_MAP_SECTOR);
        dirH->WriteBack(DIRECTORY_SECTOR);

        delete mapH;
        delete dirH;

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new UniqueOpenFile(FREE_MAP_SECTOR);
        OpenFile *directoryFile = new UniqueOpenFile(DIRECTORY_SECTOR);

        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);
        dir->WriteBack(directoryFile);

        delete directoryFile;

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            dir->Print();
        }

        delete freeMap;
        delete dir;
    } else {
        // If we are not formatting the disk, just open the freeMap file.
        freeMapFile = new UniqueOpenFile(FREE_MAP_SECTOR);
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        // Clean up files marked for deletion.
        OpenFile *directoryFile = new UniqueOpenFile(DIRECTORY_SECTOR);

        Directory *dir = new Directory(NUM_DIR_ENTRIES);
        dir->FetchFrom(directoryFile);

        // FIXME: once we implement extensible directories, this will need to change.
        // FIXME: does not handle nested directories.
        bool dirty = false;
        for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
            unsigned sector = dir->GetRaw()->table[i].sector;

            if (dir->IsMarkedForDeletion(sector)) {
                DEBUG('f', "Removing file marked for deletion at sector %u.\n", sector);

                FileHeader *hdr = new FileHeader;
                hdr->FetchFrom(sector);
                hdr->Deallocate(freeMap);
                delete hdr;

                freeMap->Clear(sector);

                ASSERT(dir->RemoveMarkedForDeletion(sector));

                dirty = true;
            }
        }

        if (dirty) {
            freeMap->WriteBack(freeMapFile);
            dir->WriteBack(directoryFile);
        }

        delete directoryFile;

        delete freeMap;
        delete dir;
    }
}

FileSystem::~FileSystem() {
    delete lock;
    delete freeMapFile;
    delete openFileManager;
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
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool FileSystem::CreateFile(const char *filepath, unsigned initialSize) {
    ASSERT(filepath != nullptr);
    ASSERT(initialSize < MAX_FILE_SIZE);

    // TODO: ideally, we should acquire a lock only for what we use:
    // - the cwd
    // - the freemap
    lock->Acquire();

    DEBUG('f', "Creating file %s, size %u\n", filepath, initialSize);

    // TODO: check that directory is not marked for deletion.
    int directorySector = LoadDirectory(filepath);
    if (directorySector == -1) {
        lock->Release();

        return false;
    }

    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(GetSynchOpenFile(directorySector));  // FIXME: free and decrement reference count

    const char *name = GetFileName(filepath);

    bool success;
    if (dir->HasEntry(name)) {
        success = false;  // File or directory with the same name already exists.
    } else {
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);
        int sector = freeMap->Find();
        // Find a sector to hold the file header.
        if (sector == -1) {
            success = false;  // No free block for file header.
        } else if (!dir->Add(name, sector)) {
            success = false;  // No space in directory.
        } else {
            FileHeader *h = new FileHeader;
            success = h->Allocate(freeMap, initialSize);
            // Fails if no space on disk for data.
            if (success) {
                // Everything worked, flush all changes back to disk.
                h->WriteBack(sector);
                dir->WriteBack(GetSynchOpenFile(directorySector));  // FIXME: unmanage and free synchopenfile
                freeMap->WriteBack(freeMapFile);
            }
            delete h;
        }
        delete freeMap;
    }

    delete dir;

    lock->Release();

    return success;
}

bool FileSystem::CreateDirectory(const char *path) {
    ASSERT(path != nullptr);

    lock->Acquire();

    // TODO: check that directory is not marked for deletion.
    int directorySector = LoadDirectory(path);
    if (directorySector == -1) {
        lock->Release();

        return false;
    }

    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(GetSynchOpenFile(directorySector)); // FIXME:

    const char *name = GetFileName(path);
    bool success;

    if (dir->HasEntry(name)) {
        success = false;  // File or directory with the same name already exists.
    } else {
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        int sector = freeMap->Find();
        if (sector == -1) {
            success = false;
        } else {
            FileHeader *fileHeader = new FileHeader;
            if (!fileHeader->Allocate(freeMap, DIRECTORY_FILE_SIZE)) {
                success = false;
            } else {
                Directory *newDir = new Directory(NUM_DIR_ENTRIES);

                if (!dir->Add(name, sector, true)) {
                    success = false;
                } else {
                    OpenFile *newDirectoryFile = new OpenFile(sector, fileHeader);
                    newDir->WriteBack(newDirectoryFile);

                    fileHeader->WriteBack(sector);

                    delete newDirectoryFile;

                    fileHeader->WriteBack(sector);
                    dir->WriteBack(GetSynchOpenFile(directorySector)); // FIXME:
                    freeMap->WriteBack(freeMapFile);

                    success = true;
                }

                delete newDir;
            }

            delete fileHeader;
        }

        delete freeMap;
    }

    delete dir;

    lock->Release();

    return success;
}

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *FileSystem::Open(const char *filepath) {
    ASSERT(filepath != nullptr);

    lock->Acquire();

    DEBUG('f', "Opening file %s\n", filepath);

    // TODO: check that directory is not marked for deletion.
    int directorySector = LoadDirectory(filepath);
    if (directorySector == -1) {
        lock->Release();

        return nullptr;
    }

    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(GetSynchOpenFile(directorySector));

    const char *name = GetFileName(filepath);
    int sector = dir->FindFile(name);
    if (sector == -1) {
        delete dir;

        lock->Release();

        return nullptr;
    }

    delete dir;

    if (!openFileManager->IsManaged(sector)) {
        RWLock *rwLock = new RWLock();
        FileHeader *fileHeader = new FileHeader;
        fileHeader->FetchFrom(sector);

        openFileManager->Manage(sector, 0, rwLock, fileHeader);
    }

    openFileManager->IncrementReferenceCount(sector);

    OpenFile *file = new SynchOpenFile(sector, openFileManager->GetFileHeader(sector), openFileManager->GetRWLock(sector));

    lock->Release();

    return file;
}

void FileSystem::Close(OpenFile *file) {
    lock->Acquire();

    DEBUG('f', "Closing file %u\n", file->GetSector());

    unsigned referenceCount = openFileManager->DecrementReferenceCount(file->GetSector());

    if (referenceCount == 0) {
        DEBUG('f', "Closing last reference to file at sector %u.\n", file->GetSector());

        openFileManager->Unmanage(file->GetSector());

        // TODO: check that directory is not marked for deletion.
        Directory *dir = new Directory(NUM_DIR_ENTRIES);
        dir->FetchFrom(currentThread->GetCWD());

        if (dir->IsMarkedForDeletion(file->GetSector())) {
            DEBUG('f', "File at sector %u is marked for deletion.\n", file->GetSector());

            FreeFile(file->GetSector());

            ASSERT(dir->RemoveMarkedForDeletion(file->GetSector()));
            dir->WriteBack(currentThread->GetCWD());  // Flush to disk.
        }

        delete dir;
    }

    delete file;

    lock->Release();
}

/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool FileSystem::RemoveFile(const char *filepath) {
    ASSERT(filepath != nullptr);

    lock->Acquire();

    DEBUG('f', "Removing file %s\n", filepath);

    // TODO: check that directory is not marked for deletion.
    int directorySector = LoadDirectory(filepath);
    if (directorySector == -1) {
        lock->Release();

        return false;
    }
    
    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(GetSynchOpenFile(directorySector)); // FIXME:

    const char *name = GetFileName(filepath);
    int sector = dir->FindFile(name);
    if (sector == -1) {
        delete dir;

        lock->Release();

        return false;
    }

    if (openFileManager->GetReferenceCount(sector) > 0) {
        DEBUG('f', "File %s is open, marking for deletion.\n", name);

        dir->MarkForDeletion(name);
        dir->WriteBack(GetSynchOpenFile(directorySector)); // FIXME:

        delete dir;

        lock->Release();

        return true;
    }

    FreeFile(sector);

    ASSERT(dir->Remove(name));
    dir->WriteBack(GetSynchOpenFile(directorySector));  // Flush to disk. FIXME:

    delete dir;

    lock->Release();

    return true;
}

bool FileSystem::RemoveDirectory(const char *path) {
    ASSERT(path != nullptr);

    lock->Acquire();

    DEBUG('f', "Removing directory %s\n", path);

    // TODO: check that directory is not marked for deletion.
    // TODO: allow removing a directory in a subdirectory.

    int directorySector = LoadDirectory(path);
    if (directorySector == -1) {
        lock->Release();

        return false;
    }

    Directory *cwd = new Directory(NUM_DIR_ENTRIES);
    cwd->FetchFrom(GetSynchOpenFile(directorySector)); // FIXME: ?

    const char* name = GetFileName(path);
    int sector = cwd->FindDirectory(name);
    if (sector == -1) {
        delete cwd;

        lock->Release();

        return false;
    }

    // FIXME: should be using a SynchOpenFile instead.
    OpenFile *file = new UniqueOpenFile(sector);
    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(file);
    delete file;

    if (!dir->IsEmpty()) {
        delete dir;
        delete cwd;

        lock->Release();

        return false;
    }

    delete dir;

    if (openFileManager->GetReferenceCount(sector) > 0) {
        DEBUG('f', "Directory %s is open, marking for deletion.\n", path);

        cwd->MarkForDeletion(name);
        cwd->WriteBack(GetSynchOpenFile(directorySector));

        delete cwd;

        lock->Release();

        return true;
    }

    FreeFile(sector);

    ASSERT(cwd->Remove(name));
    cwd->WriteBack(GetSynchOpenFile(directorySector));  // Flush to disk.

    delete cwd;

    lock->Release();

    return true;
}

bool FileSystem::ExtendFile(unsigned sector, unsigned bytes) {
    ASSERT(openFileManager->IsManaged(sector));

    lock->Acquire();

    FileHeader *fileHeader = openFileManager->GetFileHeader(sector);

    Bitmap *bitmap = new Bitmap(NUM_SECTORS);
    bitmap->FetchFrom(freeMapFile);

    bool success = fileHeader->Extend(bitmap, bytes);

    if (success) {
        bitmap->WriteBack(freeMapFile);
        fileHeader->WriteBack(sector);
    }

    delete bitmap;

    lock->Release();

    return success;
}

void FileSystem::FreeFile(unsigned sector) {
    ASSERT(lock->IsHeldByCurrentThread());

    FileHeader *fileH = new FileHeader;
    fileH->FetchFrom(sector);

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);

    fileH->Deallocate(freeMap);  // Remove data blocks.

    freeMap->Clear(sector);           // Remove header block.
    freeMap->WriteBack(freeMapFile);  // Flush to disk.

    delete fileH;
    delete freeMap;
}

/// List files in a directory.
char *FileSystem::ListDirectoryContents(const char *path) {
    lock->Acquire();

    // TODO: check that directory is not marked for deletion.
    // TODO: allow listing a directory in a subdirectory.
    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    dir->FetchFrom(currentThread->GetCWD());

    if (path != nullptr) {
        int directorySector = LoadDirectory(path);
        dir->FetchFrom(GetSynchOpenFile(directorySector));
        const char* name = GetFileName(path);

        int sector = dir->FindDirectory(name);
        if (sector == -1) {
            delete dir;

            lock->Release();

            return nullptr;
        }

        // FIXME: this should open a SynchOpenFile instead.
        OpenFile *file = new UniqueOpenFile(sector);
        dir->FetchFrom(file);

        delete file;
    }

    char *contents = dir->ListContents();

    delete dir;

    lock->Release();

    return contents;
}

bool FileSystem::ChangeDirectory(const char *path) {
    lock->Acquire();

    // TODO: check that directory is not marked for deletion.
    // TODO: allow changing to a directory in a subdirectory.
    Directory *cwd = new Directory(NUM_DIR_ENTRIES);
    cwd->FetchFrom(currentThread->GetCWD());

    if (path == nullptr) {
        OpenFile *file = currentThread->GetCWD();

        unsigned referenceCount = openFileManager->DecrementReferenceCount(file->GetSector());

        if (referenceCount == 0) {
            DEBUG('f', "Closing last reference to file at sector %u.\n", file->GetSector());

            openFileManager->Unmanage(file->GetSector());

            if (cwd->IsMarkedForDeletion(file->GetSector())) {
                DEBUG('f', "File at sector %u is marked for deletion.\n", file->GetSector());

                FreeFile(file->GetSector());

                ASSERT(cwd->RemoveMarkedForDeletion(file->GetSector()));
                cwd->WriteBack(currentThread->GetCWD());  // Flush to disk.
            }
        }

        delete file;

        delete cwd;

        currentThread->SetCWD(GetRootDirectory());

        lock->Release();

        return true;
    }

    int directorySector = LoadDirectory(path);
    if (directorySector == -1)
    {
        delete cwd;

        lock->Release();
        
        return false;
    }
    cwd->FetchFrom(GetSynchOpenFile(directorySector));

    const char* name = GetFileName(path);
    int sector = cwd->FindDirectory(name);
    if (sector == -1) {
        delete cwd;

        lock->Release();

        return false;
    }

    // cd dir1/dir2

    // TODO: check if subdirectory is marked for deletion before changing.
    // TODO: refactor, this is duplicated above and in some other methods
    OpenFile *file = GetSynchOpenFile(sector);

    unsigned referenceCount = openFileManager->DecrementReferenceCount(file->GetSector());

    if (referenceCount == 0) {
        DEBUG('f', "Closing last reference to file at sector %u.\n", file->GetSector());

        openFileManager->Unmanage(file->GetSector());

        if (cwd->IsMarkedForDeletion(file->GetSector())) {
            DEBUG('f', "File at sector %u is marked for deletion.\n", file->GetSector());

            FreeFile(file->GetSector());

            ASSERT(cwd->RemoveMarkedForDeletion(file->GetSector()));
            cwd->WriteBack(GetSynchOpenFile(directorySector));  // Flush to disk.
        }
    }

    delete file;
    delete cwd;

    currentThread->SetCWD(GetSynchOpenFile(sector));

    lock->Release();

    return true;
}

SynchOpenFile *FileSystem::GetSynchOpenFile(unsigned sector) {
    // TODO: assert lock is held?

    if (!openFileManager->IsManaged(sector)) {
        RWLock *rwLock = new RWLock();
        FileHeader *fileHeader = new FileHeader;
        fileHeader->FetchFrom(sector);

        openFileManager->Manage(sector, 0, rwLock, fileHeader);
    }

    openFileManager->IncrementReferenceCount(sector);

    return new SynchOpenFile(sector, openFileManager->GetFileHeader(sector), openFileManager->GetRWLock(sector));
}

SynchOpenFile *FileSystem::GetRootDirectory() { return GetSynchOpenFile(DIRECTORY_SECTOR); }

// FIXME: do not use uniqueopenfile
// TODO: remove unused arg
int FileSystem::LoadDirectory(const char *path) {
    Directory *dir = new Directory(NUM_DIR_ENTRIES);
    if (path == nullptr) {
        OpenFile *file = GetRootDirectory();
        dir->FetchFrom(file);

        // TODO: make sure to decrement count too, has to happen every time a synch open file is freed
        delete file;

        return -1; // TODO: figure out what the proper return value is here
    }

    dir->FetchFrom(currentThread->GetCWD());
    int sector = currentThread->GetCWD()->GetSector();

    char *buffer = CopyString(path);
    char *cp = buffer;

    char *token = strtok_r(buffer, "/", &buffer);
    while (token != nullptr) {
        char *nextToken = strtok_r(nullptr, "/", &buffer);
        if (nextToken == nullptr) break;

        sector = dir->FindDirectory(token);
        if (sector == -1) {
            delete cp;
            delete dir;

            return -1;
        }

        OpenFile *file = new UniqueOpenFile(sector);
        dir->FetchFrom(file);

        delete file;

        token = nextToken;
    }

    delete cp;
    delete dir;

    return sector;
}

static bool AddToShadowBitmap(unsigned sector, Bitmap *map) {
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool CheckForError(bool value, const char *message) {
    if (!value) {
        DEBUG('f', "Error: %s\n", message);
    }
    return !value;
}

static bool CheckSector(unsigned sector, Bitmap *shadowMap) {
    if (CheckForError(sector < NUM_SECTORS, "sector number too big.  Skipping bitmap check.")) {
        return true;
    }
    return CheckForError(AddToShadowBitmap(sector, shadowMap), "sector number already used.");
}

static bool CheckFileHeader(const RawFileHeader *rh, unsigned num, Bitmap *shadowMap) {
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f',
          "Checking file header %u.  File size: %u bytes, number of sectors: "
          "%u.\n",
          num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes, SECTOR_SIZE), "sector count not compatible with file size.");
    error |= CheckForError(rh->numSectors < NUM_DIRECT, "too many blocks.");
    for (unsigned i = 0; i < rh->numSectors; i++) {
        unsigned s = rh->dataSectors[i];
        error |= CheckSector(s, shadowMap);
    }
    return error;
}

// static bool CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap) {
//     bool error = false;
//     for (unsigned i = 0; i < NUM_SECTORS; i++) {
//         DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n", i, freeMap->Test(i), shadowMap->Test(i));
//         error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i), "inconsistent bitmap.");
//     }
//     return error;
// }
//
// static bool CheckDirectory(const RawDirectory *rd, Bitmap *shadowMap) {
//     ASSERT(rd != nullptr);
//     ASSERT(shadowMap != nullptr);
//
//     bool error = false;
//     unsigned nameCount = 0;
//     const char *knownNames[NUM_DIR_ENTRIES];
//
//     for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
//         DEBUG('f', "Checking direntry: %u.\n", i);
//         const DirectoryEntry *e = &rd->table[i];
//
//         if (e->inUse) {
//             if (strlen(e->name) > FILE_NAME_MAX_LEN) {
//                 DEBUG('f', "Filename too long.\n");
//                 error = true;
//             }
//
//             // Check for repeated filenames.
//             DEBUG('f', "Checking for repeated names.  Name count: %u.\n", nameCount);
//             bool repeated = false;
//             for (unsigned j = 0; j < nameCount; j++) {
//                 DEBUG('f', "Comparing \"%s\" and \"%s\".\n", knownNames[j], e->name);
//                 if (strcmp(knownNames[j], e->name) == 0) {
//                     DEBUG('f', "Repeated filename.\n");
//                     repeated = true;
//                     error = true;
//                 }
//             }
//             if (!repeated) {
//                 knownNames[nameCount] = e->name;
//                 DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
//                 nameCount++;
//             }
//
//             // Check sector.
//             error |= CheckSector(e->sector, shadowMap);
//
//             // Check file header.
//             FileHeader *h = new FileHeader;
//             const RawFileHeader *rh = h->GetRaw();
//             h->FetchFrom(e->sector);
//             error |= CheckFileHeader(rh, e->sector, shadowMap);
//             delete h;
//         }
//     }
//     return error;
// }

bool FileSystem::Check() {
    lock->Acquire();

    DEBUG('f', "Performing filesystem check\n");
    bool error = false;

    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader *bitH = new FileHeader;
    const RawFileHeader *bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f',
          "  File size: %u bytes, expected %u bytes.\n"
          "  Number of sectors: %u, expected %u.\n",
          bitRH->numBytes, FREE_MAP_FILE_SIZE, bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE, "bad bitmap header: wrong file size.");
    error |= CheckForError(bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE, "bad bitmap header: wrong number of sectors.");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    // DEBUG('f', "Checking directory.\n");
    //
    // FileHeader *dirH = new FileHeader;
    // const RawFileHeader *dirRH = dirH->GetRaw();
    // dirH->FetchFrom(DIRECTORY_SECTOR);
    // error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    // delete dirH;
    //
    // Directory *dir = new Directory(NUM_DIR_ENTRIES);
    // const RawDirectory *rdir = dir->GetRaw();
    // dir->FetchFrom(directoryFile);
    // error |= CheckDirectory(rdir, shadowMap);
    // delete dir;
    //
    // Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    // freeMap->FetchFrom(freeMapFile);
    //
    // // The two bitmaps should match.
    // DEBUG('f', "Checking bitmap consistency.\n");
    // error |= CheckBitmaps(freeMap, shadowMap);
    // delete shadowMap;
    // delete freeMap;

    DEBUG('f', error ? "Filesystem check failed.\n" : "Filesystem check succeeded.\n");

    lock->Release();

    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void FileSystem::Print() {
    lock->Acquire();

    FileHeader *bitH = new FileHeader;
    FileHeader *dirH = new FileHeader;
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    Directory *dir = new Directory(NUM_DIR_ENTRIES);

    printf("--------------------------------\n");
    bitH->FetchFrom(FREE_MAP_SECTOR);
    bitH->Print("Bitmap");

    printf("--------------------------------\n");
    dirH->FetchFrom(DIRECTORY_SECTOR);
    dirH->Print("Directory");

    printf("--------------------------------\n");
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    // FIXME: this should load the root directory and print its contents and its children's contents
    // printf("--------------------------------\n");
    // dir->FetchFrom(directoryFile);
    // dir->Print();
    // printf("--------------------------------\n");

    delete bitH;
    delete dirH;
    delete freeMap;
    delete dir;

    lock->Release();
}
