/// Copyright (c) 2018-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "utility.hh"

#include <cstdio>
#include <cstring>

Debug debug;

char *make_debug_name(const char *parent, const char *name) {
    if (parent == nullptr || strlen(parent) == 0) {
        return strdup(name);
    }

    char *debugName = new char[strlen(parent) + strlen(name) + 2];
    sprintf(debugName, "%s_%s", parent, name);

    return debugName;
}
