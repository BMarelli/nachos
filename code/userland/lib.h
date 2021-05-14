#ifndef USERLAND_LIB_HH
#define USERLAND_LIB_HH

#include "../userprog/syscall.h"

unsigned strlen (const char *s);

void puts2 (const char *s );

void itoa (int n , char *str );

#endif