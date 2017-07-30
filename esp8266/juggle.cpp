#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// eight colored dots, weaving in and out of sync with each other
class Juggle : public Program
{
public:
    Juggle()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        fadeToBlackBy(leds, effective_leds, 20);
        byte dothue = 0;
        for (int i = 0; i < 8; i++)
        {
            leds[beatsin16(i+7, 0, effective_leds)] |= CHSV(dothue, 200, 255);
            dothue += 32;
        }

        return true;
    }
};

REGISTER_PROGRAM(Juggle);
