/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Program base
 */

#pragma once

#include "framelimiter.hpp"

extern uint16_t autonomous_speed;

class Program
{
public:
    Program(int scale)
        : limiter(scale)
    {
    }
    
    virtual ~Program() {}

    virtual bool run() = 0;

    virtual bool allow_night_mode()
    {
        return false;
    }
    
    FrameLimiter limiter;
};

class ProgramFactory
{
public:
    ProgramFactory(const char* name) : name(name)
    {
        next = first;
        first = this;
    }

    virtual Program* launch() = 0;

    static ProgramFactory* first;
    static ProgramFactory* get(const char* name);
    ProgramFactory* next;
    const char* name;
};

template<typename T>
class ProgramFile : ProgramFactory
{
public:
  ProgramFile(const char* name) : ProgramFactory(name) {}
  virtual Program* launch()
  {
    return new T();
  }
};

#define REGISTER_PROGRAM(cls) \
  static ProgramFile<cls> cls##Program(#cls)
