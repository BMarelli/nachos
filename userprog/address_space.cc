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

    swapBitmap = new Bitmap(numPages);
#else
    ASSERT(numPages <= memoryMap->CountClear());
#endif

    DEBUG('a', "Initializing address space, num pages %u, size %u\n", numPages, size);

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;

        pageTable[i].use = false;
        pageTable[i].dirty = false;

        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
        pageTable[i].readOnly = false;

#ifdef DEMAND_LOADING
        pageTable[i].valid = false;  // Mark page as not valid initially.
#else
        int physicalPage = memoryMap->Find(this, i);
#ifdef SWAP
        if (physicalPage == -1) physicalPage = FreePageForVPN(i);
#endif
        ASSERT(physicalPage != -1);

        pageTable[i].physicalPage = physicalPage;
        pageTable[i].valid = true;
#endif
    }

#ifdef DEMAND_LOADING
    // Nothing left to do
#else
    for (unsigned i = 0; i < numPages; i++) {
        if (pageTable[i].valid) LoadPage(i);
    }
#endif
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace() {
    DEBUG('a', "Deallocating address space %p\n", this);

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
#ifdef SWAP
    if (physicalPage == -1) physicalPage = FreePageForVPN(vpn);
#endif
    ASSERT(physicalPage != -1);

    pageTable[vpn].valid = true;
    pageTable[vpn].virtualPage = vpn;
    pageTable[vpn].physicalPage = physicalPage;
    pageTable[vpn].use = false;
    pageTable[vpn].dirty = false;

    // If the code segment was entirely on a separate page, we could
    // set its pages to be read-only.
    pageTable[vpn].readOnly = false;

#ifdef SWAP
    if (swapBitmap->Test(vpn)) {
        DEBUG('a', "Loading page %u from swap file %s\n", vpn, swapFileName);

        swapFile->ReadAt(mainMemory + physicalPage * PAGE_SIZE, PAGE_SIZE, vpn * PAGE_SIZE);
        stats->numPagesLoadedFromSwap++;
        return;
    }
#endif

    DEBUG('a', "Loading page %u from executable\n", vpn);

    memset(mainMemory + physicalPage * PAGE_SIZE, 0, PAGE_SIZE);

    Executable exe(executable_file);

    uint32_t totalRead = 0;

    uint32_t codeSize = exe.GetCodeSize();
    uint32_t codeAddr = exe.GetCodeAddr();
    if (codeSize > 0 && (vpn + 1) * PAGE_SIZE >= codeAddr && vpn * PAGE_SIZE < codeAddr + codeSize) {
        uint32_t virtualAddr = Max(vpn * PAGE_SIZE, codeAddr);
        uint32_t offset = virtualAddr - codeAddr;
        uint32_t size = Min(PAGE_SIZE - (virtualAddr % PAGE_SIZE), codeSize - offset);

        int read = exe.ReadCodeBlock(mainMemory + physicalPage * PAGE_SIZE + (virtualAddr % PAGE_SIZE), size, offset);
        if (read != (int)size) {
            DEBUG('a', "Error reading segment: expected %u bytes, got %d bytes\n", size, read);
            ASSERT(false);
        }

        totalRead += size;
    }

    uint32_t initDataSize = exe.GetInitDataSize();
    uint32_t initDataAddr = exe.GetInitDataAddr();
    if (initDataSize > 0 && (vpn + 1) * PAGE_SIZE >= initDataAddr && vpn * PAGE_SIZE < initDataAddr + initDataSize) {
        uint32_t virtualAddr = Max(vpn * PAGE_SIZE, initDataAddr);
        uint32_t offset = virtualAddr - initDataAddr;
        uint32_t size = Min(PAGE_SIZE - (virtualAddr % PAGE_SIZE), initDataSize - offset);

        int read = exe.ReadDataBlock(mainMemory + physicalPage * PAGE_SIZE + (virtualAddr % PAGE_SIZE), size, offset);
        if (read != (int)size) {
            DEBUG('a', "Error reading segment: expected %u bytes, got %d bytes\n", size, read);
            ASSERT(false);
        }

        totalRead += size;
    }

    ASSERT(totalRead <= PAGE_SIZE);
}

#ifdef SWAP
void AddressSpace::SendPageToSwap(unsigned vpn) {
    ASSERT(vpn < numPages);

    DEBUG('a', "Sending page %u to swap file %s\n", vpn, swapFileName);

    // if the page is not valid, there's no need to write it to swap
    if (!pageTable[vpn].valid) return;

    pageTable[vpn].valid = false;

    // if the page is not dirty and is already in swap, it is up to date so there's no need to write it to swap
    if (!pageTable[vpn].dirty) return;

    pageTable[vpn].dirty = false;

    int written = swapFile->WriteAt(machine->GetMMU()->mainMemory + pageTable[vpn].physicalPage * PAGE_SIZE, PAGE_SIZE, vpn * PAGE_SIZE);
    if (written != PAGE_SIZE) {
        DEBUG('a', "Error writing to swap file %s: expected %u bytes, got %d bytes\n", swapFileName, PAGE_SIZE, written);
        ASSERT(false);
    }

    swapBitmap->Mark(vpn);
    stats->numPagesSentToSwap++;
}

TranslationEntry *GetEntryAtPhysicalPage(unsigned physicalPage) {
    TranslationEntry *entry = memoryMap->GetSpace(physicalPage)->GetPage(memoryMap->GetVPN(physicalPage));
    ASSERT(entry->physicalPage == physicalPage);
    ASSERT(entry->valid);

    return entry;
}

unsigned PickVictim() {
#ifdef PRPOLICY_FIFO
    static unsigned victim = -1;

    victim = (victim + 1) % NUM_PHYS_PAGES;

    return victim;
#elif PRPOLICY_CLOCK
    static unsigned hand = -1;

    currentThread->space->SaveState();

    TranslationEntry *entry;

    // use = 0, dirty = 0
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        hand = (hand + 1) % NUM_PHYS_PAGES;

        entry = GetEntryAtPhysicalPage(hand);

        if (!entry->use && !entry->dirty) return hand;
    }

    // use = 0, dirty = 1
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        hand = (hand + 1) % NUM_PHYS_PAGES;

        entry = GetEntryAtPhysicalPage(hand);

        if (!entry->use && entry->dirty) return hand;

        entry->use = false;
    #ifdef USE_TLB
        for (unsigned j = 0; j < TLB_SIZE; j++) {
            if (machine->GetMMU()->tlb[j].valid && machine->GetMMU()->tlb[j].physicalPage == hand) {
                machine->GetMMU()->tlb[j].use = false;
                break;
            }
        }
    #endif
    }

    // use = 1, dirty = 0
    for (unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        hand = (hand + 1) % NUM_PHYS_PAGES;

        entry = GetEntryAtPhysicalPage(hand);

        if (!entry->dirty) return hand;
    }

    // use = 1, dirty = 1
    hand = (hand + 1) % NUM_PHYS_PAGES;

    return hand;
#else

    return SystemDep::Random() % NUM_PHYS_PAGES;
#endif
}

unsigned AddressSpace::FreePageForVPN(unsigned vpn) {
    unsigned victim = PickVictim();
    ASSERT(victim < NUM_PHYS_PAGES);

    DEBUG('a', "Freeing page %u for virtual page %u\n", victim, vpn);

    AddressSpace *victimSpace = memoryMap->GetSpace(victim);
    ASSERT(victimSpace != nullptr);

    unsigned victimPage = memoryMap->GetVPN(victim);

#ifdef USE_TLB
    if (victimSpace == currentThread->space) {
        for (unsigned i = 0; i < TLB_SIZE; i++) {
            TranslationEntry *entry = &machine->GetMMU()->tlb[i];
            if (entry->valid && entry->virtualPage == victimPage) {
                pageTable[victimPage].use = entry->use;
                pageTable[victimPage].dirty = entry->dirty;
                entry->valid = false;
                break;
            }
        }
    }
#endif

    victimSpace->SendPageToSwap(victimPage);

    memoryMap->Mark(victim, this, vpn);

    return victim;
}
#endif
