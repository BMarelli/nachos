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

#include "disk.hh"
#include "filesys/raw_file_header.hh"
#include "lib/assert.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

FileHeader::FileHeader() {
    raw.numBytes = 0;
    raw.numSectors = 0;

    indirectDataSectors = nullptr;
    doubleIndirectSectors = nullptr;
    doubleIndirectDataSectors = nullptr;
}

FileHeader::~FileHeader() {
    if (raw.numSectors <= NUM_DIRECT) return;

    delete[] indirectDataSectors;

    if (raw.numSectors <= NUM_DIRECT + NUM_INDIRECT) return;

    delete[] doubleIndirectSectors;

    for (unsigned i = 0; i < DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT); i++) {
        delete[] doubleIndirectDataSectors[i];
    }

    delete[] doubleIndirectDataSectors;
}

/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `bitmap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool FileHeader::Allocate(Bitmap *bitmap, unsigned fileSize) {
    ASSERT(raw.numBytes == 0);
    ASSERT(raw.numSectors == 0);

    return Extend(bitmap, fileSize);
}

// Calculate the number of sectors required to store the given number of bytes
unsigned FileHeader::CalculateRequiredSectors(unsigned bytes) {
    unsigned sectors = DivRoundUp(bytes, SECTOR_SIZE);

    // Direct sectors
    unsigned requiredSectors = Min(sectors, NUM_DIRECT);

    // Indirect sectors and their containing sector
    if (sectors > NUM_DIRECT) {
        requiredSectors += 1;
        requiredSectors += Min(sectors - NUM_DIRECT, NUM_INDIRECT);
    }

    // Double indirect sectors, their containing sectors and the sector containing their containing sectors
    if (sectors > NUM_DIRECT + NUM_INDIRECT) {
        requiredSectors += 1;
        requiredSectors += DivRoundUp(sectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT);
        requiredSectors += sectors - NUM_DIRECT - NUM_INDIRECT;
    }

    return requiredSectors;
}

// Extend a file header by a number of bytes, allocating more space on disk for the file data.
bool FileHeader::Extend(Bitmap *bitmap, unsigned bytes) {
    ASSERT(bitmap != nullptr);

    if (raw.numBytes + bytes > MAX_FILE_SIZE) return false;

    // Calculate the number of additional sectors required
    unsigned requiredSectors = CalculateRequiredSectors(raw.numBytes + bytes) - CalculateRequiredSectors(raw.numBytes);

    if (bitmap->CountClear() < requiredSectors) return false;

    unsigned currentNumSectors = raw.numSectors;

    raw.numBytes += bytes;
    raw.numSectors = DivRoundUp(raw.numBytes, SECTOR_SIZE);

    // Allocate direct sectors
    if (currentNumSectors < NUM_DIRECT) {
        for (unsigned i = currentNumSectors; i < Min(raw.numSectors, NUM_DIRECT); i++) {
            raw.dataSectors[i] = bitmap->Find();
        }

        currentNumSectors = Min(raw.numSectors, NUM_DIRECT);
    }

    if (currentNumSectors == raw.numSectors) return true;

    ASSERT(currentNumSectors >= NUM_DIRECT);

    // Allocate indirect sectors
    if (currentNumSectors < NUM_DIRECT + NUM_INDIRECT) {
        if (currentNumSectors == NUM_DIRECT) {
            raw.indirectionSector = bitmap->Find();
            indirectDataSectors = new unsigned[NUM_INDIRECT];
        }

        for (unsigned i = currentNumSectors - NUM_DIRECT; i < Min(raw.numSectors - NUM_DIRECT, NUM_INDIRECT); i++) {
            indirectDataSectors[i] = bitmap->Find();
        }

        currentNumSectors = Min(raw.numSectors, NUM_DIRECT + NUM_INDIRECT);
    }

    if (currentNumSectors == raw.numSectors) return true;

    ASSERT(currentNumSectors >= NUM_DIRECT + NUM_INDIRECT);

    // Allocate double indirect sectors
    if (currentNumSectors < NUM_DIRECT + NUM_INDIRECT + NUM_INDIRECT * NUM_INDIRECT) {
        if (currentNumSectors == NUM_DIRECT + NUM_INDIRECT) {
            raw.doubleIndirectionSector = bitmap->Find();
            doubleIndirectSectors = new unsigned[NUM_INDIRECT];
            doubleIndirectDataSectors = new unsigned *[NUM_INDIRECT];
        }

        for (unsigned i = DivRoundUp(currentNumSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT);
             i < DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT); i++) {
            doubleIndirectSectors[i] = bitmap->Find();
            doubleIndirectDataSectors[i] = new unsigned[NUM_INDIRECT];
        }

        for (unsigned i = currentNumSectors - NUM_DIRECT - NUM_INDIRECT;
             i < Min(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT * NUM_INDIRECT); i++) {
            doubleIndirectDataSectors[i / NUM_INDIRECT][i % NUM_INDIRECT] = bitmap->Find();
        }

        currentNumSectors = Min(raw.numSectors, NUM_DIRECT + NUM_INDIRECT + NUM_INDIRECT * NUM_INDIRECT);
    }

    ASSERT(currentNumSectors == raw.numSectors);

    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `bitmap` is the bit map of free disk sectors.
void FileHeader::Deallocate(Bitmap *bitmap) {
    ASSERT(bitmap != nullptr);

    // Deallocate direct sectors
    for (unsigned i = 0; i < Min(raw.numSectors, NUM_DIRECT); i++) {
        ASSERT(bitmap->Test(raw.dataSectors[i]));  // ought to be marked!
        bitmap->Clear(raw.dataSectors[i]);
    }

    if (raw.numSectors <= NUM_DIRECT) return;

    ASSERT(bitmap->Test(raw.indirectionSector));  // ought to be marked!
    bitmap->Clear(raw.indirectionSector);

    // Deallocate indirect sectors
    for (unsigned i = 0; i < Min(raw.numSectors - NUM_DIRECT, NUM_INDIRECT); i++) {
        ASSERT(bitmap->Test(indirectDataSectors[i]));  // ought to be marked!
        bitmap->Clear(indirectDataSectors[i]);
    }

    if (raw.numSectors <= NUM_DIRECT + NUM_INDIRECT) return;

    ASSERT(bitmap->Test(raw.doubleIndirectionSector));  // ought to be marked!
    bitmap->Clear(raw.doubleIndirectionSector);

    // Deallocate double indirect sectors
    for (unsigned i = 0; i < DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT); i++) {
        ASSERT(bitmap->Test(doubleIndirectSectors[i]));  // ought to be marked!
        bitmap->Clear(doubleIndirectSectors[i]);
    }

    for (unsigned i = 0; i < Min(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT * NUM_INDIRECT); i++) {
        ASSERT(bitmap->Test(doubleIndirectDataSectors[i / NUM_INDIRECT][i % NUM_INDIRECT]));  // ought to be marked!
        bitmap->Clear(doubleIndirectDataSectors[i / NUM_INDIRECT][i % NUM_INDIRECT]);
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void FileHeader::FetchFrom(unsigned sector) {
    synchDisk->ReadSector(sector, (char *)&raw);

    if (raw.numSectors <= NUM_DIRECT) return;

    indirectDataSectors = new unsigned[NUM_INDIRECT];
    synchDisk->ReadSector(raw.indirectionSector, (char *)indirectDataSectors);

    if (raw.numSectors <= NUM_DIRECT + NUM_INDIRECT) return;

    doubleIndirectSectors = new unsigned[NUM_INDIRECT];
    synchDisk->ReadSector(raw.doubleIndirectionSector, (char *)doubleIndirectSectors);

    doubleIndirectDataSectors = new unsigned *[NUM_INDIRECT];
    for (unsigned i = 0; i < DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT); i++) {
        doubleIndirectDataSectors[i] = new unsigned[NUM_INDIRECT];
        synchDisk->ReadSector(doubleIndirectSectors[i], (char *)doubleIndirectDataSectors[i]);
    }
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void FileHeader::WriteBack(unsigned sector) {
    synchDisk->WriteSector(sector, (char *)&raw);

    if (raw.numSectors <= NUM_DIRECT) return;

    synchDisk->WriteSector(raw.indirectionSector, (char *)indirectDataSectors);

    if (raw.numSectors <= NUM_DIRECT + NUM_INDIRECT) return;

    synchDisk->WriteSector(raw.doubleIndirectionSector, (char *)doubleIndirectSectors);

    for (unsigned i = 0; i < DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT); i++) {
        synchDisk->WriteSector(doubleIndirectSectors[i], (char *)doubleIndirectDataSectors[i]);
    }
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
