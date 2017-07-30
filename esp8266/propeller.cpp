#include "program.hpp"
#include "display.hpp"

// rgb propeller
class Propeller : public Program
{
public:
    Propeller()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        idex++;
        int thissat = 255;
        if (huecount <= 0)
        {
            huecount = 10;
            ++thishue;
        }

        int ghue = (thishue + 80) % 255;
        int bhue = (thishue + 160) % 255;
        int N3  = int(effective_leds/3);
        int N6  = int(effective_leds/6);  
        int N12 = int(effective_leds/12);  
        for (int i = 0; i < N3; i++)
        {
            int j0 = (idex + i + effective_leds - N12) % effective_leds;
            int j1 = (j0+N3) % effective_leds;
            int j2 = (j1+N3) % effective_leds;    
            leds[j0] = CHSV(thishue, thissat, 255);
            leds[j1] = CHSV(ghue, thissat, 255);
            leds[j2] = CHSV(bhue, thissat, 255);    
        }

        --huecount;
        return true;
    }

private:
    int idex = 0;
    int thishue = 0;
    int huecount = 0;
};

REGISTER_PROGRAM(Propeller);
