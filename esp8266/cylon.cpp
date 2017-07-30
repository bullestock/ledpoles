#include "program.hpp"
#include "display.hpp"

// cylon
class Cylon : public Program
{
public:
    Cylon()
        : Program(3)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        if (current_led >= effective_leds)
            current_led = 0;
        leds[current_led] = CHSV(starthue++, 255, 255);
        ++current_led;

        return true;
    }

private:
    int current_led = 0;
    int starthue = 0;
};

REGISTER_PROGRAM(Cylon);
