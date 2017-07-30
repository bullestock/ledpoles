#include "program.hpp"
#include "display.hpp"

// beats
class BeatsPerMinute : public Program
{
public:
    BeatsPerMinute()
        : Program(1)
    {
        ++starthue;
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;
        const uint8_t beat = beatsin8(62, 64, 255);
        for (int i = 0; i < effective_leds; i++)
            leds[i] = ColorFromPalette(PartyColors_p, starthue+(i*2), beat-starthue+(i*10));

        return true;
    }

private:
    static uint8_t starthue;
};

uint8_t BeatsPerMinute::starthue = 0;

REGISTER_PROGRAM(BeatsPerMinute);
