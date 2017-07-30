#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// N dots traversing the strip, changing colour once it reaches the end
class ChaseMulti : public Program
{
public:
    int idx = 0;
    bool first = true;
    ChaseMulti()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        const int N = 4;
        if (idx >= N)
            idx = 0;
        memset(leds, 0, effective_leds * 3);
        for (int i = 0; i < effective_leds; ++i)
            if (((i+idx) % N) == 0)
                leds[i] = ChaseColours::get();

        if (!idx && !first)
        {
            // done all LEDs
            ChaseColours::next();
        }
        first = false;
        ++idx;
        return true;
    }
};

REGISTER_PROGRAM(ChaseMulti);
