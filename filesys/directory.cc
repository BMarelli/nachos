/// Routines to manage a directory of file names.
///
/// The directory is a table of fixed length entries; each entry represents a
/// single file, and contains the file name, and the location of the file
/// header on disk.  The fixed size of each directory entry means that we
/// have the restriction of a fixed maximum size for file names.
///
/// The constructor initializes an empty directory of a certain size; we use
/// ReadFrom/WriteBack to fetch the contents of the directory from disk, and
/// to write back any modifications back to disk.
///
/// Also, this implementation has the restriction that the size of the
/// directory cannot expand.  In other words, once all the entries in the
/// directory are used, no more files can be created.  Fixing this is one of
/// the parts to the assignment.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "directory.hh"

#include <stdio.h>
#include <string.h>

#include "directory_entry.hh"
#include "file_header.hh"
#include "lib/assert.hh"

static const unsigned DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT = 10;

/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory() {
    raw.table = nullptr;
    raw.tableSize = 0;
}

/// De-allocate directory data structure.
Directory::~Directory() {
    if (raw.table != nullptr) delete[] raw.table;
}

/// Read the contents of the directory from disk.
///
/// * `file` is file containing the directory contents.
void Directory::FetchFrom(OpenFile *file) {
    ASSERT(raw.tableSize == 0);
    ASSERT(raw.table == nullptr);
    ASSERT(file != nullptr);
    ASSERT(file->Length() % sizeof(DirectoryEntry) == 0);

    if (file->Length() == 0) return;

    raw.tableSize = file->Length() / sizeof(DirectoryEntry);
    raw.table = new DirectoryEntry[raw.tableSize];

    int bytesRead = file->ReadAt((char *)raw.table, raw.tableSize * sizeof(DirectoryEntry), 0);

    ASSERT(bytesRead == raw.tableSize * sizeof(DirectoryEntry));
}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void Directory::WriteBack(OpenFile *file) {
    ASSERT(file != nullptr);

    if (raw.tableSize == 0) return;

    int bytesWritten = file->WriteAt((char *)raw.table, raw.tableSize * sizeof(DirectoryEntry), 0);

    ASSERT(bytesWritten == raw.tableSize * sizeof(DirectoryEntry));
}

/// Look up file name in directory, and return its location in the table of
/// directory entries.  Return -1 if the name is not in the directory.
///
/// * `name` is the file name to look up.
/// * `includeMarkedForDeletion` is whether to include files marked for deletion in the search.
int Directory::FindIndex(const char *name, bool includeMarkedForDeletion) {
    ASSERT(name != nullptr);

    for (unsigned i = 0; i < raw.tableSize; i++) {
        bool valid = raw.table[i].inUse && (includeMarkedForDeletion || !raw.table[i].markedForDeletion);

        if (valid && !strncmp(raw.table[i].name, name, FILE_NAME_MAX_LEN)) {
            return i;
        }
    }

    return -1;  // name not in directory
}

/// Look up file name in directory, and return the disk sector number where
/// the file's header is stored.  Return -1 if the name is not in the
/// directory.
///
/// * `name` is the file name to look up.
int Directory::Find(const char *name) {
    ASSERT(name != nullptr);

    int i = FindIndex(name, false);
    if (i != -1) {
        return raw.table[i].sector;
    }
    return -1;
}

/// Add a file into the directory.  Return true if successful; return false
/// if the file name is already in the directory.
///
/// * `name` is the name of the file being added.
/// * `newSector` is the disk sector containing the added file's header.
bool Directory::Add(const char *name, int newSector) {
    ASSERT(name != nullptr);

    if (FindIndex(name, false) != -1) {
        return false;
    }

    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (!raw.table[i].inUse) {
            raw.table[i].inUse = true;
            strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);
            raw.table[i].sector = newSector;
            return true;
        }
    }

    DirectoryEntry *newTable = new DirectoryEntry[raw.tableSize + DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT];
    memcpy(newTable, raw.table, raw.tableSize * sizeof(DirectoryEntry));
    delete[] raw.table;

    raw.table = newTable;
    raw.tableSize += DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT;
    for (unsigned i = raw.tableSize - DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT; i < raw.tableSize; i++) raw.table[i].inUse = false;

    raw.table[raw.tableSize - DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT].inUse = true;
    strncpy(raw.table[raw.tableSize - DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT].name, name, FILE_NAME_MAX_LEN);
    raw.table[raw.tableSize - DIRECTORY_ENTRIES_TABLE_GROWTH_INCREMENT].sector = newSector;

    return true;
}

/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
bool Directory::Remove(const char *name) {
    ASSERT(name != nullptr);

    int i = FindIndex(name, false);
    if (i == -1) {
        return false;  // name not in directory
    }
    raw.table[i].inUse = false;
    return true;
}

/// List all the file names in the directory.
void Directory::List() const {
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            printf("%s\n", raw.table[i].name);
        }
    }
}

/// List all the file names in the directory, their `FileHeader` locations,
/// and the contents of each file.  For debugging.
void Directory::Print() const {
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            printf(
                "\nDirectory entry:\n"
                "    name: %s\n"
                "    sector: %u\n",
                raw.table[i].name, raw.table[i].sector);
            hdr->FetchFrom(raw.table[i].sector);
            hdr->Print(nullptr);
        }
    }
    printf("\n");
    delete hdr;
}

const RawDirectory *Directory::GetRaw() const { return &raw; }

int Directory::FindIndexBySector(unsigned sector, bool includeMarkedForDeletion) {
    for (unsigned i = 0; i < raw.tableSize; i++) {
        bool valid = raw.table[i].inUse && (includeMarkedForDeletion || !raw.table[i].markedForDeletion);

        if (valid && raw.table[i].sector == sector) return i;
    }

    return -1;  // sector not in directory
}

// Mark a directory entry for deletion by sector.
void Directory::MarkForDeletion(unsigned sector) {
    int i = FindIndexBySector(sector, false);
    ASSERT(i != -1);  // sector must be in the directory

    raw.table[i].markedForDeletion = true;
}

// Returns whether the directory entry at a given sector is marked for deletion.
bool Directory::IsMarkedForDeletion(unsigned sector) {
    int i = FindIndexBySector(sector, true);
    ASSERT(i != -1);  // sector must be in the directory

    return raw.table[i].markedForDeletion;
}

// Remove a directory entry marked for deletion by sector.
void Directory::RemoveMarkedForDeletion(unsigned sector) {
    int i = FindIndexBySector(sector, true);
    ASSERT(i != -1 && raw.table[i].markedForDeletion);  // sector must be in the directory and marked for deletion

    raw.table[i].inUse = false;
}
