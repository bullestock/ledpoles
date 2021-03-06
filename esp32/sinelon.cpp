#include "program.hpp"
#include "display.hpp"

// no idea
class SineLon : public Program
{
public:
    SineLon()
        : Program(2)
    {
        ++starthue;
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        fadeToBlackBy(leds, effective_leds, 10);
        leds[random16(effective_leds)] += CHSV(starthue + random8(64), 200, 255);        

        return true;
    }

private:
    static uint8_t starthue;
};

uint8_t SineLon::starthue = 0;

// disabled cuz boring
//REGISTER_PROGRAM(SineLon);
