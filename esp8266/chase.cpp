#include "program.hpp"
#include "display.hpp"
#include "framelimiter.hpp"
#include "chasecolours.hpp"

class Chase : public Program
{
public:
    FrameLimiter limit;
    int idx = 0;
    bool first = true;
    Chase()
        : limit(autonomous_speed)
    {
    }

    virtual void run()
    {
        if (limit.skip()) return;
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
    }
};

REGISTER_PROGRAM(Chase);
