#include "program.hpp"
#include "display.hpp"

#// flicker effect
class Flicker : public Program
{
public:
    Flicker()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        if (huecount <= 0)
        {
            huecount = random(10, 100);
            thishue = random(0, 255);
        }
        int thissat = 255;
        int random_bright = random(0, 255);
        int random_delay = random(10, 100);
        int random_bool = random(0, random_bright);
        if (random_bool < 10)
            for (int i = 0; i < effective_leds; i++)
                leds[i] = CHSV(thishue, thissat, random_bright);

        limiter.setFps(1000/random_delay);
        --huecount;
        
        return true;
    }

private:
    int thishue = 0;
    int huecount = 0;
};

REGISTER_PROGRAM(Flicker);
