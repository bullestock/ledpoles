#include "program.hpp"
#include "display.hpp"
#include "chasecolours.hpp"

// sort of radiation symbolish
class Radiation : public Program
{
public:
    Radiation()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        int thissat = 255;
        int N3  = int(effective_leds/3);
        int N6  = int(effective_leds/6);  
        int N12 = int(effective_leds/12);  
        for (int i = 0; i < N6; i++)
        {
            tcount = tcount + .02;
            if (tcount > 3.14)
                tcount = 0.0;
            int ibright = int(sin(tcount)*255);    
            int j0 = (i + effective_leds - N12) % effective_leds;
            int j1 = (j0+N3) % effective_leds;
            int j2 = (j1+N3) % effective_leds;    
            leds[j0] = CHSV(thishue, thissat, ibright);
            leds[j1] = CHSV(thishue, thissat, ibright);
            leds[j2] = CHSV(thishue, thissat, ibright);    
        }
        ++thishue;
        
        return true;
    }

private:
    float tcount = 0.0;
    int thishue = 0;
};

REGISTER_PROGRAM(Radiation);
