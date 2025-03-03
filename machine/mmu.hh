/// DO NOT CHANGE -- part of the machine emulation
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_MMU__HH
#define NACHOS_MACHINE_MMU__HH

#include "disk.hh"
#include "exception_type.hh"
#include "statistics.hh"
#include "translation_entry.hh"

/// Definitions related to the size, and format of user memory.

const unsigned PAGE_SIZE = SECTOR_SIZE;  ///< Set the page size equal to the
                                         ///< disk sector size, for
                                         ///< simplicity.
#ifdef SWAP
const unsigned NUM_PHYS_PAGES = 20;  // We can get away with using a smaller page table if we are using swap files.
#else
const unsigned NUM_PHYS_PAGES = 128;
#endif
const unsigned MEMORY_SIZE = NUM_PHYS_PAGES * PAGE_SIZE;

/// Number of entries in the TLB, if one is present.
///
/// If there is a TLB, it will be small compared to page tables.
const unsigned TLB_SIZE = 16;

extern Statistics *stats;

/// This class simulates an MMU (memory management unit) that can use either
/// page tables or a TLB.
class MMU {
   public:
    // Initialize the MMU subsystem.
    MMU();

    // Deallocate data structures.
    ~MMU();

    /// Read or write 1, 2, or 4 bytes of virtual memory (at `addr`).  Return
    /// false if a correct translation could not be found.

    ExceptionType ReadMem(unsigned addr, unsigned size, int *value);

    ExceptionType WriteMem(unsigned addr, unsigned size, int value);

    void PrintTLB() const;

    /// Data structures -- all of these are accessible to Nachos kernel code.
    /// “Public” for convenience.
    ///
    /// Note that *all* communication between the user program and the kernel
    /// are in terms of these data structures, along with the already
    /// declared methods.

    char *mainMemory;  ///< Physical memory to store user program,
                       ///< code and data, while executing.

    /// NOTE: the hardware translation of virtual addresses in the user
    /// program to physical addresses (relative to the beginning of
    /// `mainMemory`) can be controlled by one of:
    /// * a traditional linear page table;
    /// * a software-loaded translation lookaside buffer (tlb) -- a cache of
    ///   mappings of virtual page #'s to physical page #'s.
#ifdef USE_TLB
    TranslationEntry *tlb;  ///< This pointer should be considered “read-only” to Nachos kernel code.
#else
    TranslationEntry *pageTable;
    unsigned pageTableSize;
#endif

   private:
    /// Retrieve a page entry either from a page table or the TLB.
    ExceptionType RetrievePageEntry(unsigned vpn, TranslationEntry **entry) const;

    /// Translate an address, and check for alignment.
    ///
    /// Set the use and dirty bits in the translation entry appropriately,
    /// and return an exception code if the translation could not be
    /// completed.
    ExceptionType Translate(unsigned virtAddr, unsigned *physAddr, unsigned size, bool writing);
};

#endif
