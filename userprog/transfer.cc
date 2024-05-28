/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "transfer.hh"

#include "threads/system.hh"

void ReadBufferFromUser(int userAddress, char *outBuffer, unsigned byteCount) {
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    for (unsigned i = 0; i < byteCount; i++) {
        int temp;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer++ = (unsigned char)temp;
    }
}

bool ReadStringFromUser(int userAddress, char *outString, unsigned maxByteCount) {
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    for (unsigned i = 0; i < maxByteCount; i++) {
        int temp;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        outString[i] = (unsigned char)temp;

        if (outString[i] == '\0') return true;
    }

    return false;
}

void WriteBufferToUser(const char *buffer, int userAddress, unsigned byteCount) {
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);

    for (unsigned i = 0; i < byteCount; i++) {
        ASSERT(machine->WriteMem(userAddress++, 1, (int)buffer[i]));
    }
}

void WriteStringToUser(const char *string, int userAddress) {
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    for (unsigned i = 0; string[i] != '\0'; i++) {
        ASSERT(machine->WriteMem(userAddress++, 1, (int)string[i]));
    }

    ASSERT(machine->WriteMem(userAddress, 1, (int)'\0'));
}
