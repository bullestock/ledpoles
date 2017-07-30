/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Common code
 */

#ifndef _common_hpp_
#define _common_hpp_

#ifdef __linux__
#include <time.h>
#include <unistd.h>
#else
#include <Arduino.h>
#endif

#include <stdint.h>

#ifdef __linux__
#define HZ 1000000
#else
#define HZ 80000000
#endif

inline uint32_t getCycleCount(void)
{
#ifdef __linux__
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#else
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a"(ccount));
  return ccount;
#endif
}

#ifdef __linux__
inline uint32_t millis()
{
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * uint64_t(1000) + ts.tv_nsec / 1000000;
}
#endif

#ifdef __linux__
#include <iostream>
#include <cstdarg>
class SerialPort
{
public:
  template<typename T>
  void print(const T& t)
  {
    std::cout << t;
  }
  template<typename T>
  void println(const T& t)
  {
    std::cout << t << std::endl;
  }
  void printf(const char* fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
};
extern SerialPort Serial;
#endif

#endif // _common_hpp_
