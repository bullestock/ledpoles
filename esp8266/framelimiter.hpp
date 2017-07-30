/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Frame limiter
 */

#pragma once

#include <stdint.h>

class FrameLimiter
{
public:
    FrameLimiter(int scale);
    bool skip();

    void setFps(int fps);
    
private:
    int scale;
    static int fps;
    uint32_t cyclesPerFrame;
    uint32_t nextFrame;
};

