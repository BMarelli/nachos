/// Data structures to manage a UNIX-like directory of file names.
///
/// A directory is a table of pairs: *<file name, sector #>*, giving the name
/// of each file in the directory, and where to find its file header (the
/// data structure describing where to find the file's data blocks) on disk.
///
/// We assume mutual exclusion is provided by the caller.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_DIRECTORY__HH
#define NACHOS_FILESYS_DIRECTORY__HH

#include "lib/bitmap.hh"
#include "open_file.hh"
#include "raw_directory.hh"
#include "threads/lock.hh"
#include "threads/rwlock.hh"

/// Initial file sizes for the bitmap and directory; until the file system
/// supports extensible files, the directory size sets the maximum number of
/// files that can be loaded onto the disk.
static const unsigned NUM_DIR_ENTRIES = 10;

/// The following class defines a UNIX-like “directory”.  Each entry in the
/// directory describes a file, and where to find it on disk.
///
/// The directory data structure can be stored in memory, or on disk.  When
/// it is on disk, it is stored as a regular Nachos file.
///
/// The constructor initializes a directory structure in memory; the
/// `FetchFrom`/`WriteBack` operations shuffle the directory information
/// from/to disk.
class Directory {
   public:
    /// Create an empty directory with space for `size` files.
    Directory(unsigned size);

    /// De-allocate the directory.
    ~Directory();

    void Initialize(Bitmap *bitmap);

    void Load();

    /// Find the sector number of the `FileHeader` for file: `name`.
    int Find(const char *name);

    /// Add a file name into the directory.
    bool Add(const char *name, int newSector);

    /// Remove a file from the directory.
    bool Remove(const char *name);

    /// Remove a file marked for deletion.
    bool RemoveMarkedForDeletion(unsigned sector);

    /// Mark a file for deletion.
    bool MarkForDeletion(const char *name);

    /// Determine if a file is marked for deletion.
    bool IsMarkedForDeletion(unsigned sector);

    /// Print the names of all the files in the directory.
    void List() const;

    /// Verbose print of the contents of the directory -- all the file names
    /// and their contents.
    void Print() const;

    void Begin();
    void Commit();
    void Abort();

    /// Get the raw directory structure.
    ///
    /// NOTE: this should only be used by routines that operating on the file
    /// system at a low level.
    const RawDirectory *GetRaw() const;

   private:
    bool valid;
    bool dirty;

    Lock *lock;

    FileHeader *fileHeader;
    RWLock *rwLock;

    OpenFile *file;

    RawDirectory raw;

    /// Find the index into the directory table corresponding to `name`.
    int FindIndex(const char *name, bool includeMarkedForDeletion = false);
};

#endif
