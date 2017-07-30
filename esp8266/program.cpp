/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Program base
 */

#include "program.hpp"
#include <string.h>

ProgramFactory* ProgramFactory::first = nullptr;

ProgramFactory* ProgramFactory::get(const char* name)
{
    for (ProgramFactory* factory = first; factory != nullptr; factory = factory->next)
        if (strcasecmp(factory->name, name) == 0)
            return factory;
    return nullptr;
}
