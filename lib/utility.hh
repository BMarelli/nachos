/// Miscellaneous useful definitions.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_LIB_UTILITY__HH
#define NACHOS_LIB_UTILITY__HH

/// Useful definitions for diverse data structures.

const unsigned BITS_IN_BYTE = 8;
const unsigned BITS_IN_WORD = 32;

/// Miscellaneous useful routines.

// #define min(a, b)  (((a) < (b)) ? (a) : (b))
// #define max(a, b)  (((a) > (b)) ? (a) : (b))

/// Divide and either round up or down.

template <typename T>
inline T DivRoundDown(T n, T s) {
    return n / s;
}

template <typename T>
inline T DivRoundUp(T n, T s) {
    return n / s + (n % s > 0 ? 1 : 0);
}

// make_debug_name returns a copy of a string.
char *make_debug_name(const char *name);

// make_debug_name returns the concatenation of two strings, separated by an underscore.
char *make_debug_name(const char *parent, const char *name);

// make_debug_name returns the concatenation of two strings and a number, separated by an underscore.
char *make_debug_name(const char *parent, const char *name, int index);

#endif
