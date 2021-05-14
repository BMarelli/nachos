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


#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"

#include <stdio.h>


static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

static void
ExecDummy(void* dummy) {
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();

    // machine->Run();
    // ASSERT(false); // machine->Run() never returns
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
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_EXIT: {
            // TODO: exit parece no estar funcionando como deberia? la thread termina correctamente pero
            // la maquina no se apaga.

            int status = machine->ReadRegister(4);

            DEBUG('e', "Thead `%s` exiting. Status: %d", currentThread->GetName(), status);

            currentThread->Finish(status);

            break;
        }

        case SC_EXEC: {
            DEBUG('e', "Exec, initiated by user program.\n");

            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",  FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *executable = fileSystem->Open(filename);
            if (executable == nullptr) {
                DEBUG('e', "Error: unable to open file %s.\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            AddressSpace *space = new AddressSpace(executable);
            delete executable;

            Thread *newThread = new Thread(filename, true, currentThread->GetPriority());
            newThread->space = space;

            int pid = processTable->Add(newThread);
            if (pid == -1) {
                DEBUG('e', "Error: too many processes are already running (maximum is %d).\n", processTable->SIZE);
                delete newThread;
                delete space;
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, pid);
            currentThread->Fork(ExecDummy, nullptr);
        }

        case SC_JOIN: {
            DEBUG('e', "Join, initiated by user pgrogram.\n");

            SpaceId id = (SpaceId) machine->ReadRegister(4);

            if (!processTable->HasKey(id)) {
                DEBUG('e', "Error: invalid space with id %s.\n", id);
                machine->WriteRegister(2, -1);
                break;
            }

            Thread* threadToJoin = processTable->Remove(id);
            DEBUG('e', "Thread <%s> waiting for thread <%s> to finish.\n",
                       currentThread->GetName(),
                       threadToJoin->GetName());
            int exitValue = threadToJoin->Join();
            DEBUG('e', "Thread successfully joined.\n");

            machine->WriteRegister(2, exitValue);
            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",  FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('e', "`Create` requested for file `%s`.\n", filename);

            if(fileSystem->Create(filename, 0)) {
                DEBUG('e', "File `%s` successfully created.\n", filename);
                machine->WriteRegister(2, 0);
            } else {
                DEBUG('e', "Error: file `%s` could not be created.\n", filename);
                machine->WriteRegister(2, -1);
            }

            break;
        }

        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('e', "`Remove` requested for file `%s`.\n", filename);

            if(fileSystem->Remove(filename)) {
                DEBUG('e', "File `%s` successfully removed.\n", filename);
                machine->WriteRegister(2, 0);
            } else {
                DEBUG('e', "Error: file `%s` could not be removed.\n", filename);
                machine->WriteRegister(2, -1);
            }

            break;
        }

        case SC_OPEN: { // OpenFileId Open(const char *name);
            int filenameAddr = machine->ReadRegister(4);

            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n", FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            DEBUG('e', "Opening file %s.\n", filename);

            OpenFile* file = fileSystem->Open(filename);
            if (file == nullptr) {
                DEBUG('e', "Error: file not found.\n");
                machine->WriteRegister(2, -1);
            } else {
                int id = currentThread->openFiles->Add(file);
                if (id == -1) {
                    DEBUG('e', "Error: thread <%s> already has too many open files.\n");
                    machine->WriteRegister(2, -1);
                } else {
                    id += 2;
                    DEBUG('e', "Adding file %s to <%s>'s open file table with id %d", filename, currentThread->GetName(), id);
                    machine->WriteRegister(2, id);
                }
            }

            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);

            if (fid == CONSOLE_INPUT || fid == CONSOLE_OUTPUT) {
                DEBUG('e', "Error: file with id %d can't be closed.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (fid < 0) {
                DEBUG('e', "Error: invalid OpenFileId.\n");
                machine->WriteRegister(2, -1);
            }

            if (currentThread->openFiles->HasKey(fid - 2)) {
                DEBUG('e', "File %u closed successfully.\n", fid);

                OpenFile *file = currentThread->openFiles->Remove(fid - 2);
                delete file;

                machine->WriteRegister(2, 0);
            } else {
                DEBUG('e', "File %u was not open.\n", fid);
                machine->WriteRegister(2, 0);
            }

            break;
        }

        case SC_READ: { // int Read(char *buffer, int size, OpenFileId id)
            int bufferAddr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);

            if (bufferAddr == 0) {
                DEBUG('e', "Error: address to buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (size <= 0) {
                DEBUG('e', "Error: size is not positive.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            switch(fid) {
                case CONSOLE_INPUT: {
                    DEBUG('e', "Reading %d bytes from stdin.\n", size);

                    char buffer[size + 1];

                    int i;
                    for(i = 0; i < size; i++) {
                        buffer[i] = gSynchConsole->ReadChar();
                        if(buffer[i] == EOF) break;
                    }
                    buffer[i] = '\0';

                    WriteStringToUser(buffer, bufferAddr);

                    machine->WriteRegister(2, i - 1); // return number of read bytes
                    break;
                }

                case CONSOLE_OUTPUT: {
                    DEBUG('e', "Error: tried to read from stdout.\n");
                    machine->WriteRegister(2, -1);
                    break;
                }

                default:
                    if (currentThread->openFiles->HasKey(fid - 2)) {
                        OpenFile* file = currentThread->openFiles->Get(fid - 2);

                        char buffer[size + 1];
                        int read = file->Read(buffer, sizeof buffer);
                        machine->WriteRegister(2, read);
                    } else {
                        machine->WriteRegister(2, -1);
                    }

            }

            break;
        }

        case SC_WRITE: { // int Write(const char *buffer, int size, OpenFileId id);
            int bufferAddr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);

            if (bufferAddr == 0) {
                DEBUG('e', "Error: address to buffer is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (size <= 0) {
                DEBUG('e', "Error: size is not positive.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            switch(fid) {
                case CONSOLE_INPUT: {
                    DEBUG('e', "Error: tried to write to stdin.\n");
                    machine->WriteRegister(2, -1);
                    break;
                }

                case CONSOLE_OUTPUT: {
                    DEBUG('e', "Writing %d bytes to stdout.\n", size);

                    char buffer[size + 1];
                    buffer[size] = '\0';
                    ReadStringFromUser(bufferAddr, buffer, size + 1);

                    int i;
                    for (i = 0; buffer[i] != '\0'; i++)
                        gSynchConsole->WriteChar(buffer[i]);

                    machine->WriteRegister(2, i - 1); // return number of written bytes
                    break;
                }

                default:
                    if (currentThread->openFiles->HasKey(fid - 2)) {
                        OpenFile* file = currentThread->openFiles->Get(fid - 2);

                        char buffer[size + 1];
                        buffer[size] = '\0';
                        ReadStringFromUser(bufferAddr, buffer, size + 1);

                        int i;
                        for (i = 0; buffer[i] != '\0'; i++);

                        int written = file->Write(buffer, i);
                        machine->WriteRegister(2, written);
                    } else {
                        machine->WriteRegister(2, -1);
                    }
                    machine->WriteRegister(2, 0);
            }

            break;
        }

        case SC_PS: {
            scheduler->Print();

            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
