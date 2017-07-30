/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Frame limiter
 */

#ifndef _framelimiter_hpp_
#define _framelimiter_hpp_

#include <stdint.h>

class FrameLimiter
{
public:
  FrameLimiter(int fps);
  bool skip();
private:
  uint32_t cyclesPerFrame;
  uint32_t nextFrame;
};

#endif // _framelimiter_hpp_
