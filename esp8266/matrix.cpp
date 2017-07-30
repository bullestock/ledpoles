#include "program.hpp"
#include "display.hpp"

// 
class Matrix : public Program
{
public:
    Matrix()
        : Program(2)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        // Shift up
        for (int i = effective_leds-1; i > 0; --i)
            leds[i] = leds[i-1];
        // Add a random bit at position zero
        int rand = random(0, 100);
        if (rand > 90)
            leds[0] = CRGB(0, 255, 0);
        else
            leds[0] = CRGB(0, 0, 0);

        return true;
    }
};

REGISTER_PROGRAM(Matrix);
