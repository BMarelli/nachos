/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "file_header.hh"

#include <ctype.h>
#include <stdio.h>

#include "core_map.hh"
#include "disk.hh"
#include "filesys/raw_file_header.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize) {
    ASSERT(freeMap != nullptr);

    if (fileSize > MAX_FILE_SIZE) return false;

    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);

    unsigned requiredSectors = raw.numSectors;
    if (raw.numSectors > NUM_DIRECT) requiredSectors += 1;  // Sector for indirect block.
    if (raw.numSectors > NUM_DIRECT + NUM_INDIRECT) {
        requiredSectors += 1;                                                                     // Sector for double indirect block
        requiredSectors += DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT);  // Sector for double indirect blocks
    }

    if (freeMap->CountClear() < requiredSectors) return false;

    unsigned remainingSectors = raw.numSectors;

    // Allocate direct sectors
    for (unsigned i = 0; i < Min(raw.numSectors, NUM_DIRECT); i++) raw.dataSectors[i] = freeMap->Find();
    remainingSectors -= Min(raw.numSectors, NUM_DIRECT);

    if (remainingSectors == 0) return true;

    // Allocate indirect sectors
    raw.indirectionSector = freeMap->Find();

    indirectDataSectors = new unsigned[NUM_INDIRECT];
    for (unsigned i = 0; i < Min(remainingSectors, NUM_INDIRECT); i++) indirectDataSectors[i] = freeMap->Find();
    remainingSectors -= Min(remainingSectors, NUM_INDIRECT);

    if (remainingSectors == 0) return true;

    // Allocate doubly indirect sectors
    raw.doubleIndirectionSector = freeMap->Find();

    doubleIndirectDataSectors = new unsigned *[NUM_INDIRECT];

    unsigned numDoubleIndirectSectors = DivRoundUp(remainingSectors, NUM_INDIRECT);
    for (unsigned i = 0; i < numDoubleIndirectSectors; i++) {
        doubleIndirectDataSectors[i] = new unsigned[NUM_INDIRECT];
        for (unsigned j = 0; j < Min(remainingSectors, NUM_INDIRECT); j++) doubleIndirectDataSectors[i][j] = freeMap->Find();
        remainingSectors -= Min(remainingSectors, NUM_INDIRECT);
    }

    ASSERT(remainingSectors == 0);

    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void FileHeader::Deallocate(Bitmap *freeMap) {
    ASSERT(freeMap != nullptr);

    unsigned remainingSectors = raw.numSectors;

    // Deallocate direct sectors
    for (unsigned i = 0; i < Min(raw.numSectors, NUM_DIRECT); i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!

        freeMap->Clear(raw.dataSectors[i]);
    }
    remainingSectors -= Min(remainingSectors, NUM_DIRECT);

    if (remainingSectors == 0) return;

    ASSERT(freeMap->Test(raw.indirectionSector));  // ought to be marked!

    freeMap->Clear(raw.indirectionSector);

    // Deallocate indirect sectors
    for (unsigned i = 0; i < Min(remainingSectors, NUM_INDIRECT); i++) {
        ASSERT(freeMap->Test(indirectDataSectors[i]));  // ought to be marked!

        freeMap->Clear(indirectDataSectors[i]);
    }
    remainingSectors -= Min(remainingSectors, NUM_INDIRECT);

    if (remainingSectors == 0) return;

    ASSERT(freeMap->Test(raw.doubleIndirectionSector));  // ought to be marked!

    freeMap->Clear(raw.doubleIndirectionSector);

    // Deallocate doubly indirect sectors
    unsigned numDoubleIndirectSectors = DivRoundUp(remainingSectors, NUM_INDIRECT);
    for (unsigned i = 0; i < numDoubleIndirectSectors; i++) {
        for (unsigned j = 0; j < Min(remainingSectors, NUM_INDIRECT); j++) {
            ASSERT(freeMap->Test(doubleIndirectDataSectors[i][j]));  // ought to be marked!

            freeMap->Clear(doubleIndirectDataSectors[i][j]);
        }
        remainingSectors -= Min(remainingSectors, NUM_INDIRECT);
    }

    ASSERT(remainingSectors == 0);
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void FileHeader::FetchFrom(unsigned sector) {
    synchDisk->ReadSector(sector, (char *)&raw);
    if (raw.numSectors <= NUM_DIRECT) return;

    unsigned remainingSectors = raw.numSectors - NUM_DIRECT;

    indirectDataSectors = new unsigned[NUM_INDIRECT];
    synchDisk->ReadSector(raw.indirectionSector, (char *)indirectDataSectors);
    remainingSectors -= Min(remainingSectors, NUM_INDIRECT);

    if (remainingSectors == 0) return;

    doubleIndirectDataSectors = new unsigned *[NUM_INDIRECT];
    unsigned numDoubleIndirectSectors = DivRoundUp(remainingSectors, NUM_INDIRECT);
    for (unsigned i = 0; i < numDoubleIndirectSectors; i++) {
        doubleIndirectDataSectors[i] = new unsigned[NUM_INDIRECT];
        synchDisk->ReadSector(raw.doubleIndirectionSector, (char *)doubleIndirectDataSectors[i]);

        remainingSectors -= Min(remainingSectors, NUM_INDIRECT);
    }

    ASSERT(remainingSectors == 0);
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void FileHeader::WriteBack(unsigned sector) {
    synchDisk->WriteSector(sector, (char *)&raw);

    if (raw.numSectors <= NUM_DIRECT) return;

    unsigned remainingSectors = raw.numSectors - NUM_DIRECT;

    synchDisk->WriteSector(raw.indirectionSector, (char *)indirectDataSectors);
    remainingSectors -= Min(remainingSectors, NUM_INDIRECT);

    if (remainingSectors == 0) return;

    unsigned numDoubleIndirectSectors = DivRoundUp(remainingSectors, NUM_INDIRECT);
    for (unsigned i = 0; i < numDoubleIndirectSectors; i++) {
        synchDisk->WriteSector(raw.doubleIndirectionSector, (char *)doubleIndirectDataSectors[i]);

        remainingSectors -= Min(remainingSectors, NUM_INDIRECT);
    }

    ASSERT(remainingSectors == 0);
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned FileHeader::ByteToSector(unsigned offset) {
    ASSERT(offset < raw.numBytes);

    return GetSector(offset / SECTOR_SIZE);
}

/// Return the number of bytes in the file.
unsigned FileHeader::FileLength() const { return raw.numBytes; }

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void FileHeader::Print(const char *title) {
    char data[SECTOR_SIZE];

    if (title == nullptr) {
        printf("File header:\n");
    } else {
        printf("%s file header:\n", title);
    }

    printf(
        "    size: %u bytes\n"
        "    block indexes: ",
        raw.numBytes);

    for (unsigned i = 0; i < raw.numSectors; i++) printf("%u ", GetSector(i));
    printf("\n");

    for (unsigned i = 0, k = 0; i < raw.numSectors; i++) {
        unsigned sector = GetSector(i);

        printf("    contents of block %u:\n", sector);

        synchDisk->ReadSector(sector, data);

        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j])) {
                printf("%c", data[j]);
            } else {
                printf("\\%X", (unsigned char)data[j]);
            }
        }
        printf("\n");
    }
}

const RawFileHeader *FileHeader::GetRaw() const { return &raw; }

unsigned FileHeader::GetSector(unsigned i) {
    ASSERT(i < raw.numSectors);

    if (i < NUM_DIRECT) return raw.dataSectors[i];

    if (i < NUM_DIRECT + NUM_INDIRECT) return indirectDataSectors[i - NUM_DIRECT];

    return doubleIndirectDataSectors[(i - NUM_DIRECT - NUM_INDIRECT) / NUM_INDIRECT][(i - NUM_DIRECT - NUM_INDIRECT) % NUM_INDIRECT];
}
