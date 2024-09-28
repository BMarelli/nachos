/// Miscellaneous useful definitions.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_LIB_UTILITY__HH
#define NACHOS_LIB_UTILITY__HH

#include <cstring>

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

inline char* CopyString(const char* source) {
    unsigned len = strlen(source) + 1;
    char* dest = new char[len];
    strncpy(dest, source, len);

    return dest;
}

template <typename T>
inline T Min(T a, T b) {
    return a < b ? a : b;
}

template <typename T>
inline T Max(T a, T b) {
    return a > b ? a : b;
}

// Find the last occurrence of a character in a string.
// Returns the index of the character in the string, or -1 if not found.
//
// * `source` is the string to search.
// * `c` is the character to search for.
inline int FindLast(const char* source, char c) {
    unsigned i = strlen(source);
    while (i > 0 && source[i - 1] != c) --i;

    return i - 1;
}

#endif
