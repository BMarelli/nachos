/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

void ReadBufferFromUser(int userAddress, char *outBuffer, unsigned byteCount) {
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    for(unsigned i = 0; i < byteCount; i++) {
        int temp;
        if (!machine->ReadMem(userAddress++, 1, &temp));
#ifdef USE_TLB
            machine->ReadMem(userAddress - 1, 1, &temp);
#else
            ASSERT(false);
#endif
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    }
}

bool ReadStringFromUser(int userAddress, char *outString, unsigned maxByteCount){
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        if (!machine->ReadMem(userAddress++, 1, &temp));
#ifdef USE_TLB
            machine->ReadMem(userAddress - 1, 1, &temp);
#else
            ASSERT(false);
#endif
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress, unsigned byteCount) {
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);

    for(unsigned i = 0; i < byteCount; i++) {
        if (!machine->WriteMem(userAddress++, 1, buffer[i]));
#ifdef USE_TLB
            machine->WriteMem(userAddress - 1, 1, buffer[i]);
#else
            ASSERT(false);
#endif
    }
}

void WriteStringToUser(const char *string, int userAddress) {
    ASSERT(string != nullptr);
    ASSERT(userAddress != 0);

    unsigned i = 0;
    do {
        if (!machine->WriteMem(userAddress++, 1, (int) string[i]));
#ifdef USE_TLB
            machine->WriteMem(userAddress - 1, 1, (int) string[i]);
#else
            ASSERT(false);
#endif
    } while (*string++ != '\0');
}
