/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH

#include "machine/disk.hh"

static const unsigned NUM_DIRECT = (SECTOR_SIZE - 4 * sizeof(unsigned)) / sizeof(unsigned);
static const unsigned NUM_INDIRECT = SECTOR_SIZE / sizeof(unsigned);

const unsigned MAX_FILE_SIZE = (NUM_DIRECT + NUM_INDIRECT + NUM_INDIRECT * NUM_INDIRECT) * SECTOR_SIZE;

struct RawFileHeader {
    unsigned numBytes;                 ///< Number of bytes in the file.
    unsigned numSectors;               ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data block in this file.
    unsigned indirectionSector;        ///< Disk sector number for the indirect block.
    unsigned doubleIndirectionSector;  ///< Disk sector number for the double indirect block.
};

#endif
