/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Scalable Counter
 */

#include "common.hpp"

// Generic scalable tick counter with compile-time choice of RATE
// (ticks per second) and PERIOD (ticks before wrap-around).
class Counter
{
public:
    Counter(int r, int p)
        : rate(r),
          period(p)
    {
    }

    uint32_t operator()()
    {
        uint32_t current = getCycleCount();
        const uint32_t divisor = HZ / rate;
        residual += current - previous;
        counter = (counter + residual / divisor) % period;
        residual %= divisor;
        previous = current;
        return counter;
    }

private:
    int rate;
    int period;
    uint32_t counter = 0;
    uint32_t residual = 0;
    uint32_t previous = 0;
};
