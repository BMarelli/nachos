/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

// DUDA: las strings no deberian ser null terminated?
// en la linea de ReadStringFromUser
// while (*outString++ != '\0' ...
// esta asumiendo que el que usa la cadena ya puso el nulo al final?
// pero cuando hacemos write a userspace, deberiamos agregarlo?

void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    for(unsigned i = 0; i < byteCount; i++) {
        int temp;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    }
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(buffer != nullptr);
    ASSERT(userAddress != 0);
    ASSERT(byteCount != 0);

    for(unsigned i = 0; i < byteCount; i++) {
        ASSERT(machine->WriteMem(userAddress, 1, (int) buffer[i]));
        buffer++;
    }

}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(string != nullptr);
    ASSERT(userAddress != 0);

    unsigned i = 0;
    do {
        ASSERT(machine->WriteMem(userAddress, 1, (int) string[i]));
        userAddress++;
    } while (*string++ != '\0');
}
