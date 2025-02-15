/// DO NOT CHANGE -- part of the machine emulation
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2018-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_EXCEPTIONTYPE__HH
#define NACHOS_MACHINE_EXCEPTIONTYPE__HH

enum ExceptionType {
    NO_EXCEPTION,             ///< Everything ok!
    SYSCALL_EXCEPTION,        ///< A program executed a system call.
    PAGE_FAULT_EXCEPTION,     ///< No valid translation found.
    READ_ONLY_EXCEPTION,      ///< Write attempted to page marked
                              ///< “read-only”.
    BUS_ERROR_EXCEPTION,      ///< Translation resulted in an invalid
                              ///< physical address.
    ADDRESS_ERROR_EXCEPTION,  ///< Unaligned reference or one that was beyond
                              ///< the end of the address space.
    OVERFLOW_EXCEPTION,       ///< Integer overflow in `add` or `sub`.
    ILLEGAL_INSTR_EXCEPTION,  ///< Unimplemented or reserved instruction.
    NUM_EXCEPTION_TYPES
};

/// Produce a textual name of an exception type.
///
/// Useful for debugging exceptions generated by user program execution.
const char *ExceptionTypeToString(ExceptionType et);

#endif
