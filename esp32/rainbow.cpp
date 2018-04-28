#include "program.hpp"
#include "display.hpp"

// rainbow gliding across the strip
class Rainbow : public Program
{
public:
    Rainbow()
        : Program(10)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;
        fill_rainbow(leds, effective_leds, --starthue, 20);
        return true;
    }

private:
    uint8_t starthue = 0;
};

REGISTER_PROGRAM(Rainbow);
