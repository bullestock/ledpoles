#include "program.hpp"
#include "display.hpp"
#include "counter.hpp"
#include "framelimiter.hpp"
#include "chasecolours.hpp"

class Chase : public Program
{
public:
    FrameLimiter limit;
    Counter c;
    bool first = true;
    Chase()
        : limit(50),
          c(10 /* !! */, effective_leds)
    {
    }

    virtual void run()
    {
        if (limit.skip()) return;
        const auto idx = c();
        if (idx)
            leds[idx-1] = CRGB::Black;
        else if (!first)
        {
            // done all LEDs
            ChaseColours::next();
        }
        leds[idx] = ChaseColours::get();
        first = false;
    }
};

REGISTER_PROGRAM(Chase);
