#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// growing/receding bars
class Bars : public Program
{
public:
    int idx = 0;
    bool first = true;
    bool forwards = true;
    Bars()
        : Program(3)
    {
    }

    bool allow_night_mode() override
    {
        return true;
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        clear_all();
        if (idx >= effective_leds)
        {
            idx = effective_leds-1;
            forwards = false;
        }
        else if (idx < 0)
        {
            idx = 0;
            forwards = true;
        }
        if (!idx && !first)
        {
            // done all LEDs
            ChaseColours::next();
        }
        for (int i = 0; i <= idx; ++i)
            leds[i] = ChaseColours::get();
        first = false;
        if (forwards)
            ++idx;
        else
            --idx;
        return true;
    }
};

REGISTER_PROGRAM(Bars);
