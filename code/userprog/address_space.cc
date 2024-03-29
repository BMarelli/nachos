/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
  ASSERT(executable_file != nullptr);

  Executable exe (executable_file);
  ASSERT(exe.CheckMagic());

#ifdef DEMAND_LOADING
  exec_file = executable_file;
#endif

  // How big is address space?
  unsigned size = exe.GetSize() + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
  numPages = DivRoundUp(size, PAGE_SIZE);
  size = numPages * PAGE_SIZE;

  // TODO: only if no demand loading / swap
  ASSERT(numPages <= memoryBitmap->CountClear());

  DEBUG('a', "Initializing address space, num pages %u, size %u\n",
        numPages, size);

  char *mainMemory = machine->GetMMU()->mainMemory;

  // First, set up the translation.

  pageTable = new TranslationEntry[numPages];
  for (unsigned i = 0; i < numPages; i++) {
#ifdef DEMAND_LOADING
    pageTable[i].virtualPage  = -1;
    pageTable[i].physicalPage = -1;
#else
    int free = memoryBitmap->Find();

    if (free < 0) {
      DEBUG('a', "Error: could not find a free physical page");
      break;

      // TODO: swap???
    }
    pageTable[i].virtualPage  = i;
    pageTable[i].physicalPage = free;
    memset(mainMemory + free * PAGE_SIZE, 0, PAGE_SIZE);
#endif
    pageTable[i].valid        = true;
    pageTable[i].use          = false;
    pageTable[i].dirty        = false;
    pageTable[i].readOnly     = false;
      // If the code segment was entirely on a separate page, we could
      // set its pages to be read-only.

    }

#ifndef DEMAND_LOADING
    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr();
        DEBUG('a', "Initializing code segment. Size: %u.\n", codeSize);

        for (uint32_t i = 0; i < codeSize; i++) {
          uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
          uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
          uint32_t physAddr = frame * PAGE_SIZE + offset;

          exe.ReadCodeBlock(&(mainMemory[physAddr]), 1, i);
        }
    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment. Size %u\n", initDataSize);

        for (uint32_t i = 0; i < initDataSize; i++) {
          uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
          uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
          uint32_t physAddr = frame * PAGE_SIZE + offset;

          exe.ReadDataBlock(&(mainMemory[physAddr]), 1, i);
        }
   }
#endif
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
  for (unsigned int i = 0; i < numPages; i++) {
    memoryBitmap->Clear(pageTable[i].physicalPage);
    memset(&machine->GetMMU()->mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE);
  }

  delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
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
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
#ifndef USE_TLB
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
#else
    MMU* mmu = machine->GetMMU();
    for (unsigned i = 0; i < TLB_SIZE; i++) {
      mmu->tlb[i].valid = false;
    }
#endif
}

TranslationEntry*
AddressSpace::GetPageTable() {
  return pageTable;
}

#ifdef DEMAND_LOADING
void AddressSpace::LoadPage(unsigned vpn) {
  int free = memoryBitmap->Find();
  if (free < 0) {
    DEBUG('e', "Error: could not find a free physical page\n");
    // TODO: swap???
  }

  char *mainMemory = machine->GetMMU()->mainMemory;
  memset(mainMemory + free * PAGE_SIZE, 0, PAGE_SIZE);

  Executable exe (exec_file);

  unsigned physAddr = free * PAGE_SIZE;
  unsigned virtualAddr = vpn * PAGE_SIZE;
  uint32_t codeSize = exe.GetCodeSize();

  unsigned read = 0;
  if (codeSize > 0 && virtualAddr < codeSize) {
    unsigned size = codeSize - virtualAddr < PAGE_SIZE ? codeSize - virtualAddr : PAGE_SIZE;
    exe.ReadCodeBlock(&mainMemory[physAddr], size, virtualAddr);

    read += size;
  }

  uint32_t initDataSize = exe.GetInitDataSize();
  uint32_t initDataAddr = exe.GetInitDataAddr();
  if (read < PAGE_SIZE && initDataSize > 0 && virtualAddr + read < initDataAddr + initDataSize) {
    unsigned size = (initDataSize - virtualAddr < PAGE_SIZE ? initDataSize - virtualAddr : PAGE_SIZE) - read;
    exe.ReadDataBlock(&mainMemory[physAddr + read], size, virtualAddr + read);

    read += size;
  }

  ASSERT(read <= PAGE_SIZE);

  pageTable[vpn].physicalPage = free;
  pageTable[vpn].valid = true;
}
#endif
