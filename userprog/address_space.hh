/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRESSSPACE__HH
#define NACHOS_USERPROG_ADDRESSSPACE__HH

#include "machine/translation_entry.hh"
#include "userprog/executable.hh"

#ifdef SWAP
#include "filesys/directory_entry.hh"
#include "lib/bitmap.hh"
#endif

/// ReadBlockFunction is a pointer to a member function of Executable that
/// reads a block of data from a segment of the executable file.
using ReadBlockFunction = int (Executable::*)(char *, unsigned, unsigned);

const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!

class AddressSpace {
   public:
    /// Create an address space to run a user program.
    ///
    /// The address space is initialized from an already opened file.
    /// The program contained in the file is loaded into memory and
    /// everything is set up so that user instructions can start to be
    /// executed.
    ///
    /// Parameters:
    /// * `executable_file` is the open file that corresponds to the
    ///   program; it contains the object code to load into memory.
    /// * `pid` is the process identifier of the process that is running
    ///   this address space.
    AddressSpace(OpenFile *executable_file, int pid);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    void RestoreState();

    /// Retrieve a page from the page table.
    TranslationEntry *GetPage(unsigned vpn);

    /// Load a page and mark it as valid.
    void LoadPage(unsigned vpn);

#ifdef SWAP
    /// Send a page to the swap file.
    void SendPageToSwap(unsigned vpn);
#endif

   private:
    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

    /// Number of pages in the virtual address space.
    unsigned numPages;

    /// The executable file that contains the object code.
    OpenFile *executable_file;

    /// The process identifier of the process that is running this address space.
    int pid;

#ifdef SWAP
    /// Free a page for a given virtual page number.
    unsigned FreePageForVPN(unsigned vpn);

    /// Indicates which pages are in the swap file.
    Bitmap *swapBitmap;

    /// The name of the swap file.
    char swapFileName[FILE_NAME_MAX_LEN + 1];

    /// The swap file for the address space.
    OpenFile *swapFile;
#endif
};

#endif
