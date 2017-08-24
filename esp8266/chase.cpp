#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// a single dot traversing the strip, changing colour once it reaches the end
class Chase : public Program
{
public:
    int idx = 0;
    bool first = true;
    Chase()
        : Program(2)
    {
    }

    bool allow_night_mode() override
    {
        return true;
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;
        if (idx >= effective_leds)
            idx = 0;
        const auto last_idx = idx ? idx-1 : effective_leds-1;
        leds[last_idx] = CRGB::Black;
        if (!idx && !first)
        {
            // done all LEDs
            ChaseColours::next();
        }
        leds[idx] = ChaseColours::get();
        first = false;
        ++idx;
        return true;
    }
};

REGISTER_PROGRAM(Chase);
