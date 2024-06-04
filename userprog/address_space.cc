/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "address_space.hh"

#include <string.h>

#include "executable.hh"
#include "lib/debug.hh"
#include "lib/utility.hh"
#include "mmu.hh"
#include "threads/system.hh"

#ifdef SWAP
#include <cstdio>

#include "system_dep.hh"
#endif

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *_executable_file, int _pid) {
    ASSERT(_executable_file != nullptr);
    ASSERT(_pid != -1);

    executable_file = _executable_file;
    pid = _pid;

    Executable exe(executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

#ifdef SWAP
    sprintf(swapFileName, "SWAP.%d", pid);

    ASSERT(fileSystem->Create(swapFileName, 0));
    ASSERT((swapFile = fileSystem->Open(swapFileName)) != nullptr);
#else
    ASSERT(numPages <= memoryMap->CountClear());
#endif

    DEBUG('a', "Initializing address space, num pages %u, size %u\n", numPages, size);

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].use = false;
        pageTable[i].dirty = false;

        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
        pageTable[i].readOnly = false;

#ifdef DEMAND_LOADING
        pageTable[i].valid = false;  // Mark pages as not present initially
#else
        int physicalPage = memoryMap->Find(this, i);
        ASSERT(physicalPage != -1);

        pageTable[i].physicalPage = physicalPage;
        pageTable[i].valid = true;

        memset(machine->GetMMU()->mainMemory + physicalPage * PAGE_SIZE, 0, PAGE_SIZE);
#endif
    }

#ifdef DEMAND_LOADING
    // We're done.
#else
    // Load the code and data segments into memory.

    uint32_t codeSize = exe.GetCodeSize();
    uint32_t codeAddr = exe.GetCodeAddr();
    if (codeSize > 0) {
        DEBUG('a', "Initializing code segment at 0x%X with size %u\n", codeAddr, codeSize);
        loadSegment(exe, codeAddr, codeSize, &Executable::ReadCodeBlock);
    }

    uint32_t initDataSize = exe.GetInitDataSize();
    uint32_t initDataAddr = exe.GetInitDataAddr();
    if (initDataSize > 0) {
        DEBUG('a', "Initializing data segment at 0x%X with size %u\n", initDataAddr, initDataSize);
        loadSegment(exe, initDataAddr, initDataSize, &Executable::ReadDataBlock);
    }
#endif
}

void AddressSpace::loadSegment(Executable &exe, uint32_t virtualAddr, uint32_t segmentSize, ReadBlockFunction readBlock) {
    char *mainMemory = machine->GetMMU()->mainMemory;

    uint32_t totalRead = 0;
    while (totalRead < segmentSize) {
        uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);
        uint32_t pageOffset = virtualAddr % PAGE_SIZE;

        char *dest = mainMemory + pageTable[virtualPage].physicalPage * PAGE_SIZE + pageOffset;
        uint32_t size = Min(segmentSize - totalRead, PAGE_SIZE - pageOffset);
        uint32_t offset = totalRead;

        int read = (exe.*readBlock)(dest, size, offset);
        if (read != (int)size) {
            DEBUG('a', "Error reading segment: expected %u bytes, got %d bytes\n", size, read);
            ASSERT(false);
        }

        virtualAddr += size;
        totalRead += size;
    }
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace() {
    for (unsigned i = 0; i < numPages; i++) {
        if (pageTable[i].valid) {
            memoryMap->Clear(pageTable[i].physicalPage);
        }
    }

    delete[] pageTable;
    delete executable_file;

#ifdef SWAP
    if (!fileSystem->Remove(swapFileName)) {
        DEBUG('a', "Error removing swap file %s\n", swapFileName);
    }

    delete swapFile;
#endif
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void AddressSpace::InitRegisters() {
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n", numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
void AddressSpace::SaveState() {
#ifdef USE_TLB
    TranslationEntry *tlb = machine->GetMMU()->tlb;

    for (unsigned i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid) {
            pageTable[tlb[i].virtualPage].use = tlb[i].use;
            pageTable[tlb[i].virtualPage].dirty = tlb[i].dirty;
        }
    }
#endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState() {
#ifdef USE_TLB
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        machine->GetMMU()->tlb[i].valid = false;
    }
#else
    machine->GetMMU()->pageTable = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
#endif
}

TranslationEntry *AddressSpace::GetPage(unsigned vpn) {
    ASSERT(vpn < numPages);

    return &pageTable[vpn];
}

void AddressSpace::LoadPage(unsigned vpn) {
    ASSERT(vpn < numPages);

    char *mainMemory = machine->GetMMU()->mainMemory;

    int physicalPage = memoryMap->Find(this, vpn);
    ASSERT(physicalPage != -1);

    pageTable[vpn].virtualPage = vpn;
    pageTable[vpn].physicalPage = physicalPage;
    pageTable[vpn].valid = true;
    pageTable[vpn].use = false;
    pageTable[vpn].dirty = false;
    pageTable[vpn].readOnly = false;

    memset(mainMemory + physicalPage * PAGE_SIZE, 0, PAGE_SIZE);

    Executable exe(executable_file);

    uint32_t totalRead = 0;

    uint32_t codeSize = exe.GetCodeSize();
    uint32_t codeAddr = exe.GetCodeAddr();
    if (codeSize > 0 && (vpn + 1) * PAGE_SIZE > codeAddr && vpn * PAGE_SIZE < codeAddr + codeSize) {
        uint32_t virtualAddr = Max(vpn * PAGE_SIZE, codeAddr);
        uint32_t offset = virtualAddr - codeAddr;
        uint32_t size = Min(PAGE_SIZE - (virtualAddr % PAGE_SIZE), codeSize - offset);

        int read = exe.ReadCodeBlock(mainMemory + physicalPage * PAGE_SIZE + (virtualAddr % PAGE_SIZE), size, offset);
        if (read != (int)size) {
            DEBUG('a', "Error reading segment: expected %u bytes, got %d bytes\n", size, read);
            ASSERT(false);
        }

        totalRead += read;
    }

    uint32_t initDataSize = exe.GetInitDataSize();
    uint32_t initDataAddr = exe.GetInitDataAddr();
    if (initDataSize > 0 && (vpn + 1) * PAGE_SIZE > initDataAddr && vpn * PAGE_SIZE < initDataAddr + initDataSize) {
        uint32_t virtualAddr = Max(vpn * PAGE_SIZE, initDataAddr);
        uint32_t offset = virtualAddr - initDataAddr;
        uint32_t size = Min(PAGE_SIZE - (virtualAddr % PAGE_SIZE), initDataSize - offset);

        int read = exe.ReadDataBlock(mainMemory + physicalPage * PAGE_SIZE + (virtualAddr % PAGE_SIZE), size, offset);
        if (read != (int)size) {
            DEBUG('a', "Error reading segment: expected %u bytes, got %d bytes\n", size, read);
            ASSERT(false);
        }

        totalRead += read;
    }

    ASSERT(totalRead <= PAGE_SIZE);
}
#ifdef SWAP
unsigned PickVictim() { return SystemDep::Random() % NUM_PHYS_PAGES; }
#endif
