#include "program.hpp"
#include "display.hpp"

// a single dot traversing the strip, changing colour once it reaches the end
class RandomBurst : public Program
{
public:
    RandomBurst()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        int thissat = 255;
        auto idex = random(0, effective_leds);
        auto ihue = random(0, 255);  
        leds[idex] = CHSV(ihue, thissat, 255);

        return true;
    }
};

REGISTER_PROGRAM(RandomBurst);
