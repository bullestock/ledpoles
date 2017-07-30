#pragma once

#include "display.hpp"

class ChaseColours
{
public:
    static CRGB get()
    {
        return chase_colours[idx];
    }

    static void next();

private:
    static int idx;
    static const CRGB chase_colours[];
};
