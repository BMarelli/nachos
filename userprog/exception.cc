/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include <stdio.h>

const unsigned USER_STRING_MAX_LEN = 128;

#include "address_space.hh"
#include "args.hh"
#include "filesys/directory_entry.hh"
#include "lib/debug.hh"
#include "lib/utility.hh"
#include "machine.hh"
#include "syscall.h"
#include "threads/system.hh"
#include "transfer.hh"

static void IncrementPC() {
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

static bool ReadStringFromUserAtRegister(unsigned num, char *buffer, unsigned size) {
    int addr = machine->ReadRegister(num);

    if (addr == 0) {
        DEBUG('e', "Error: address to user string is null.\n");
        return false;
    }

    if (!ReadStringFromUser(addr, buffer, size)) {
        DEBUG('e', "Error: string too long (maximum is %u bytes).\n", sizeof buffer);
        return false;
    }

    return true;
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void DefaultHandler(ExceptionType et) {
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n", ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

static void ExecProcess(void *args) {
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();

    if (args) {
        unsigned argc = WriteArgs((char **)args);
        machine->WriteRegister(4, argc);

        int argv = machine->ReadRegister(STACK_REG);
        machine->WriteRegister(5, argv);

        // NOTE: we substract 24 bytes to make room for the function
        // call argument area as mandated by the MIPS ABI.
        // ref: WriteArgs (userprog/args.hh)
        machine->WriteRegister(STACK_REG, argv - 24);
    }

    machine->Run();
    ASSERT(false);  // machine->Run() never returns
}

static void HandleHalt() {
    DEBUG('e', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
}

static void HandleJoin() {
    SpaceId pid = machine->ReadRegister(4);

    if (pid < 0) {
        DEBUG('e', "Error: invalid process id %d.\n", pid);
        machine->WriteRegister(2, -1);
        return;
    }

    DEBUG('e', "Thread `%s` requested to join with process %d.\n", currentThread->GetName(), pid);

    if (!processTable->HasKey(pid)) {
        DEBUG('e', "Error: process %d does not exist.\n", pid);
        machine->WriteRegister(2, -1);
        return;
    }

    int status = processTable->Get(pid)->Join();

    processTable->Remove(pid);

    DEBUG('e', "Thread `%s` joined with process %d with status %d.\n", currentThread->GetName(), pid, status);

    machine->WriteRegister(2, status);
}

static void HandleExec() {
    char filepath[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, filepath, sizeof filepath)) {
        machine->WriteRegister(2, -1);
        return;
    }

    OpenFile *executable = fileSystem->Open(filepath);
    if (executable == nullptr) {
        DEBUG('e', "Error: file `%s` not found.\n", filepath);

        machine->WriteRegister(2, -1);
        return;
    }

    Thread *thread = new Thread(filepath, true, currentThread->GetPriority());

    SpaceId pid = processTable->Add(thread);
    if (pid == -1) {
        DEBUG('e', "Error: too many processes are already running (maximum is %u).\n", processTable->SIZE);

        delete thread;

        machine->WriteRegister(2, -1);
        return;
    }

    thread->space = new AddressSpace(executable, pid);

    int argsAddr = machine->ReadRegister(5);
    char **args = (argsAddr == 0) ? nullptr : SaveArgs(argsAddr);

#ifdef FILESYS
    if (currentThread->GetCurrentWorkingDirectory() != nullptr) {
        unsigned directorySector = currentThread->GetCurrentWorkingDirectory()->GetSector();

        OpenFile *directoryFile = new OpenFile(directorySector, new FileHeader);
        directoryFile->GetFileHeader()->FetchFrom(directorySector);

        thread->SetCurrentWorkingDirectory(directoryFile);
    }
#endif

    thread->Fork(ExecProcess, args);

    machine->WriteRegister(2, pid);
}

static void HandleExit() {
    int status = machine->ReadRegister(4);

    DEBUG('e', "Thread `%s` exited with status %d.\n", currentThread->GetName(), status);

    currentThread->Finish(status);
}

static void HandleCreate() {
    char filepath[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, filepath, sizeof filepath)) {
        machine->WriteRegister(2, -1);
        return;
    }

    DEBUG('e', "`Create` requested for file `%s`.\n", filepath);

    if (!fileSystem->CreateFile(filepath, 0)) {
        DEBUG('e', "Error: file `%s` could not be created.\n", filepath);

        machine->WriteRegister(2, -1);
        return;
    }

    DEBUG('e', "File `%s` created.\n", filepath);

    machine->WriteRegister(2, 0);
}

static void HandleRemove() {
    char filepath[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, filepath, sizeof filepath)) {
        machine->WriteRegister(2, -1);

        return;
    }

    DEBUG('e', "`Remove` requested for file `%s`.\n", filepath);

    if (!fileSystem->RemoveFile(filepath)) {
        DEBUG('e', "Error: file `%s` could not be removed.\n", filepath);
        machine->WriteRegister(2, -1);

        return;
    }

    DEBUG('e', "File `%s` removed.\n", filepath);

    machine->WriteRegister(2, 0);
}

static void HandleOpen() {
    char filepath[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, filepath, sizeof filepath)) {
        machine->WriteRegister(2, -1);
        return;
    }

    DEBUG('e', "`Open` requested for file `%s`.\n", filepath);

    OpenFile *file = fileSystem->Open(filepath);
    if (file == nullptr) {
        DEBUG('e', "Error: file `%s` not found.\n", filepath);

        machine->WriteRegister(2, -1);
        return;
    }

    int key = currentThread->openFiles->Add(file);
    if (key == -1) {
        DEBUG('e', "Error: too many open files (maximum is %u).\n", currentThread->openFiles->SIZE);

        machine->WriteRegister(2, -1);
        return;
    }

    OpenFileId fid = key + 2;

    DEBUG('e', "File `%s` opened with id %d.\n", filepath, fid);

    machine->WriteRegister(2, fid);
}

static void HandleClose() {
    OpenFileId fid = machine->ReadRegister(4);

    if (fid < 0) {
        DEBUG('e', "Error: invalid file id %d.\n", fid);
        machine->WriteRegister(2, -1);
        return;
    }

    DEBUG('e', "`Close` requested for id %d.\n", fid);

    switch (fid) {
        case CONSOLE_INPUT: {
            DEBUG('e', "Error: cannot close console input.\n");

            machine->WriteRegister(2, -1);
            return;
        }

        case CONSOLE_OUTPUT: {
            DEBUG('e', "Error: cannot close console output.\n");

            machine->WriteRegister(2, -1);
            return;
        }

        default: {
            int key = fid - 2;

            if (!currentThread->openFiles->HasKey(key)) {
                DEBUG('e', "Error: file with id %d does not exist.\n", fid);

                machine->WriteRegister(2, -1);
                return;
            }

            fileSystem->Close(currentThread->openFiles->Remove(key));

            DEBUG('e', "File with id %d closed.\n", fid);

            machine->WriteRegister(2, 0);
            break;
        }
    }
}

static void HandleRead() {
    int bufferAddr = machine->ReadRegister(4);
    unsigned int size = machine->ReadRegister(5);
    OpenFileId fid = machine->ReadRegister(6);

    if (bufferAddr == 0) {
        DEBUG('e', "Error: address to buffer is null.\n");

        machine->WriteRegister(2, -1);
        return;
    }

    if (size == 0) {
        DEBUG('e', "Error: size must be greater than 0.\n");

        machine->WriteRegister(2, -1);
        return;
    }

    if (fid < 0) {
        DEBUG('e', "Error: invalid file id %d.\n", fid);

        machine->WriteRegister(2, -1);
        return;
    }

    switch (fid) {
        case CONSOLE_INPUT: {
            DEBUG('e', "Reading from console input.\n");

            char *buffer = new char[size];
            int bytesRead = synchConsole->Read(buffer, size);

            if (bytesRead > 0) WriteBufferToUser(buffer, bufferAddr, bytesRead);

            delete[] buffer;

            machine->WriteRegister(2, bytesRead);

            break;
        }

        case CONSOLE_OUTPUT: {
            DEBUG('e', "Error: cannot read from console output.\n");

            machine->WriteRegister(2, -1);
            break;
        }

        default: {
            int key = fid - 2;

            if (!currentThread->openFiles->HasKey(key)) {
                DEBUG('e', "Error: file with id %d does not exist.\n", fid);

                machine->WriteRegister(2, -1);
                return;
            }

            OpenFile *file = currentThread->openFiles->Get(key);

            char *buffer = new char[size];
            int bytesRead = file->Read(buffer, size);

            if (bytesRead > 0) WriteBufferToUser(buffer, bufferAddr, bytesRead);

            delete[] buffer;

            machine->WriteRegister(2, bytesRead);
            break;
        }
    }
}

static void HandleWrite() {
    int bufferAddr = machine->ReadRegister(4);
    unsigned int size = machine->ReadRegister(5);
    OpenFileId fid = machine->ReadRegister(6);

    if (bufferAddr == 0) {
        DEBUG('e', "Error: address to buffer is null.\n");

        machine->WriteRegister(2, -1);
        return;
    }

    if (size == 0) {
        DEBUG('e', "Error: size must be greater than 0.\n");

        machine->WriteRegister(2, -1);
        return;
    }

    if (fid < 0) {
        DEBUG('e', "Error: invalid file id %d.\n", fid);

        machine->WriteRegister(2, -1);
        return;
    }

    switch (fid) {
        case CONSOLE_INPUT: {
            DEBUG('e', "Error: cannot write to console input.\n");

            machine->WriteRegister(2, -1);
            break;
        }

        case CONSOLE_OUTPUT: {
            DEBUG('e', "Writing to console output.\n");

            char *buffer = new char[size];
            ReadBufferFromUser(bufferAddr, buffer, size);

            synchConsole->Write(buffer, size);

            delete[] buffer;

            machine->WriteRegister(2, size);
            break;
        }

        default: {
            int key = fid - 2;

            if (!currentThread->openFiles->HasKey(key)) {
                DEBUG('e', "Error: file with id %d does not exist.\n", fid);

                machine->WriteRegister(2, -1);
                return;
            }

            OpenFile *file = currentThread->openFiles->Get(key);

            char *buffer = new char[size];
            ReadBufferFromUser(bufferAddr, buffer, size);

            int bytesWritten = file->Write(buffer, size);

            delete[] buffer;

            machine->WriteRegister(2, bytesWritten);
            break;
        }
    }
}

static void HandlePS() {
    DEBUG('e', "Process list requested.\n");

    scheduler->Print();
}

static void HandleChangeDirectory() {
    bool hasArgument = machine->ReadRegister(4) != 0;
    char path[USER_STRING_MAX_LEN + 1];

    if (hasArgument) {
        if (!ReadStringFromUserAtRegister(4, path, sizeof path)) {
            machine->WriteRegister(2, -1);

            return;
        }
    }

    if (!fileSystem->ChangeDirectory(hasArgument ? path : nullptr)) {
        machine->WriteRegister(2, -1);

        return;
    }

    machine->WriteRegister(2, 0);
}

static void HandleCreateDirectory() {
    char path[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, path, sizeof path)) {
        machine->WriteRegister(2, -1);
        return;
    }

    if (!fileSystem->CreateDirectory(path)) {
        machine->WriteRegister(2, -1);
        return;
    }

    machine->WriteRegister(2, 0);
}

static void HandleListDirectoryContents() {
    bool hasArgument = machine->ReadRegister(4) != 0;
    char path[USER_STRING_MAX_LEN + 1];

    if (hasArgument) {
        if (!ReadStringFromUserAtRegister(4, path, sizeof path)) {
            machine->WriteRegister(2, -1);

            return;
        }
    }

    char *contents = fileSystem->ListDirectoryContents(hasArgument ? path : nullptr);
    if (contents == nullptr) {
        machine->WriteRegister(2, -1);

        return;
    }

    synchConsole->Write(contents, strlen(contents));

    delete contents;

    machine->WriteRegister(2, 0);
}

void static HandleRemoveDirectory() {
    char path[USER_STRING_MAX_LEN + 1];
    if (!ReadStringFromUserAtRegister(4, path, sizeof path)) {
        machine->WriteRegister(2, -1);

        return;
    }

    if (!fileSystem->RemoveDirectory(path)) {
        machine->WriteRegister(2, -1);

        return;
    }

    machine->WriteRegister(2, 0);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void SyscallHandler(ExceptionType _et) {
    int scid = machine->ReadRegister(2);

    switch (scid) {
        case SC_HALT:
            HandleHalt();
            break;

        case SC_EXIT:
            HandleExit();
            break;

        case SC_CREATE:
            HandleCreate();
            break;

        case SC_REMOVE:
            HandleRemove();
            break;

        case SC_OPEN:
            HandleOpen();
            break;

        case SC_CLOSE:
            HandleClose();
            break;

        case SC_READ:
            HandleRead();
            break;

        case SC_WRITE:
            HandleWrite();
            break;

        case SC_JOIN:
            HandleJoin();
            break;

        case SC_EXEC:
            HandleExec();
            break;

        case SC_PS:
            HandlePS();
            break;

        case SC_CHANGEDIRECTORY:
            HandleChangeDirectory();
            break;

        case SC_CREATEDIRECTORY:
            HandleCreateDirectory();
            break;

        case SC_LISTDIRECTORYCONTENTS:
            HandleListDirectoryContents();
            break;

        case SC_REMOVEDIRECTORY:
            HandleRemoveDirectory();
            break;

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);
    }

    IncrementPC();
}

#ifdef USE_TLB
TranslationEntry *PickTLBVictim() {
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        if (!machine->GetMMU()->tlb[i].valid) {
            return &machine->GetMMU()->tlb[i];
        }
    }

    static unsigned tlbIndex = 0;
    TranslationEntry *entry = &machine->GetMMU()->tlb[tlbIndex];
    tlbIndex = (tlbIndex + 1) % TLB_SIZE;

    return entry;
}
#endif

static void PageFaultHandler(ExceptionType et) {
    stats->numPageFaults++;

    unsigned badVAddr = machine->ReadRegister(BAD_VADDR_REG);

    DEBUG('e', "Page fault at address 0x%X.\n", badVAddr);

    unsigned vpn = DivRoundDown(badVAddr, PAGE_SIZE);

    TranslationEntry *page = currentThread->space->GetPage(vpn);
    if (!page->valid) {
        DEBUG('e', "Page not valid. Loading from disk.\n");

        currentThread->space->LoadPage(vpn);
    }

#ifdef USE_TLB
    TranslationEntry *entry = PickTLBVictim();

    if (entry->valid) {
        TranslationEntry *victimPage = currentThread->space->GetPage(entry->virtualPage);
        victimPage->use = entry->use;
        victimPage->dirty = entry->dirty;
    }

    *entry = *page;
#endif
}

static void ReadOnlyHandler(ExceptionType et) {
    unsigned badVAddr = machine->ReadRegister(BAD_VADDR_REG);

    DEBUG('e', "Read-only exception at address 0x%X.\n", badVAddr);

    fprintf(stderr, "Read-only exception at address 0x%X. Terminating process.\n", badVAddr);
    currentThread->Finish(-1);
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void SetExceptionHandlers() {
    machine->SetHandler(NO_EXCEPTION, &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION, &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION, &PageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION, &ReadOnlyHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
