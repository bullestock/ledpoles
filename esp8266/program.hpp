/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Program base
 */

#ifndef _program_hpp_
#define _program_hpp_

class Program
{
public:
  virtual ~Program() {}
  virtual void run() = 0;
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

#endif // _program_hpp_
