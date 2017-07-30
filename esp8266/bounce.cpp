#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// a single dot traversing the strip, changing colour once it reaches the end
class Bounce : public Program
{
public:
    int idx = 0;
    bool first = true;
    bool forwards = true;
    Bounce()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;
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
        const auto last_idx = forwards ? (idx ? idx-1 : effective_leds-1) :
           (idx < effective_leds-1) ? idx+1 : 0;
        leds[last_idx] = CRGB::Black;
        if (!idx && !first)
        {
            // done all LEDs
            ChaseColours::next();
        }
        leds[idx] = ChaseColours::get();
        first = false;
        if (forwards)
            ++idx;
        else
            --idx;
        return true;
    }
};

REGISTER_PROGRAM(Bounce);
