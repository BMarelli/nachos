/// Copyright (c) 2018-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "utility.hh"

#include <cstdio>
#include <cstring>

char *make_debug_name(const char *name) { return strdup(name); }

char *make_debug_name(const char *parent, const char *name) {
    char *debugName = new char[strlen(parent) + strlen(name) + 2];
    sprintf(debugName, "%s_%s", parent, name);

    return debugName;
}

char *make_debug_name(const char *parent, const char *name, int index) {
    char *debugName = new char[strlen(parent) + strlen(name) + 16];
    sprintf(debugName, "%s_%s_%d", parent, name, index);

    return debugName;
}
