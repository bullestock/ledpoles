#include "program.hpp"
#include "display.hpp"

// emergency lights (strobe left/right)
class Emergency : public Program
{
public:
    Emergency()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        int thishue = 0;
        int thathue = (thishue + 160) % 255;
        int thissat = 255;
        const int TOP_INDEX = int(effective_leds/2);
        if (state < 10)
        {
            if (!(state % 2))
            {
                // Even: Set to red
                for (int i = 0 ; i < TOP_INDEX; i++)
                    leds[i] = CHSV(thishue, thissat, 255);
            }
            else
            {
                // Odd: Blank
                clear_all();
            }
        }
        else
        {
            if (!(state % 2))
            {
                // Even: Set to blue
                for (int i = TOP_INDEX ; i < effective_leds; i++)
                    leds[i] = CHSV(thathue, thissat, 255);
            }
            else
            {
                // Odd: Blank
                clear_all();
            }
        }
        ++state;
        if (state > 20)
            state = 0;
        
        return true;
    }

private:
    int state = 0;
};

REGISTER_PROGRAM(Emergency);
