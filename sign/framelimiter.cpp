/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Frame limiter
 */

#include "framelimiter.hpp"
#include "common.hpp"

FrameLimiter::FrameLimiter(int s)
    : scale(s)
{
    nextFrame = getCycleCount();
    setFps(fps);
}

void FrameLimiter::setFps(int f)
{
    fps = f;
    cyclesPerFrame = HZ / (scale*fps);
}

bool FrameLimiter::skip()
{
  uint32_t now = getCycleCount();
  uint32_t untilNextFrame = nextFrame - now;
  if (untilNextFrame < cyclesPerFrame)
      return true;

  nextFrame += cyclesPerFrame;
  untilNextFrame = nextFrame - now;
  if (untilNextFrame >= cyclesPerFrame)
      nextFrame = now + cyclesPerFrame;

  return false;
}

int FrameLimiter::fps = 10;
