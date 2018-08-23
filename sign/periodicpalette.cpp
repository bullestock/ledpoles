#include "program.hpp"
#include "display.hpp"

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};


// periodically changing palette
class PeriodicPalette : public Program
{
public:
    int idx = 0;
    bool first = true;
    PeriodicPalette()
        : Program(1)
    {
    }

    virtual bool run()
    {
        if (limiter.skip()) return false;

        // Change palette every 8 seconds
        const int secondHand = millis()/8000;
        static int lastSecond = 999;
    
        if (lastSecond != secondHand)
        {
            lastSecond = secondHand;
            const int pal = random(11);
            switch (pal)
            {
            case 0:  currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; break;
            case 1:  currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  break;
            case 2:  currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; break;
            case 3:  SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; break;
            case 4:  SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; break;
            case 5:  SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; break;
            case 6:  SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; break;
            case 7:  currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; break;
            case 8:  currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; break;
            case 9:  currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  break;
            case 10: currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; break;
            }
        }
    
        ++startIndex;
    
        const uint8_t brightness = 255;

        auto colorIndex = startIndex;
        for (int i = 0; i < effective_leds; i++)
        {
            leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
            colorIndex += 3;
        }

        return true;
    }

private:
    // This function fills the palette with totally random colors.
    void SetupTotallyRandomPalette()
    {
        for (int i = 0; i < 16; i++)
            currentPalette[i] = CHSV(random8(), 255, random8());
    }

    // This function sets up a palette of black and white stripes,
    // using code.  Since the palette is effectively an array of
    // sixteen CRGB colors, the various fill_* functions can be used
    // to set them up.
    void SetupBlackAndWhiteStripedPalette()
    {
        // 'black out' all 16 palette entries...
        fill_solid(currentPalette, 16, CRGB::Black);
        // and set every fourth one to white.
        currentPalette[0] = CRGB::White;
        currentPalette[4] = CRGB::White;
        currentPalette[8] = CRGB::White;
        currentPalette[12] = CRGB::White;
    }

    // This function sets up a palette of purple and green stripes.
    void SetupPurpleAndGreenPalette()
    {
        CRGB purple = CHSV(HUE_PURPLE, 255, 255);
        CRGB green  = CHSV(HUE_GREEN, 255, 255);
        CRGB black  = CRGB::Black;
    
        currentPalette = CRGBPalette16(green,  green,  black,  black,
                                       purple, purple, black,  black,
                                       green,  green,  black,  black,
                                       purple, purple, black,  black);
    }

    int startIndex = 0;
    CRGBPalette16 currentPalette;
    TBlendType    currentBlending = LINEARBLEND;
};

REGISTER_PROGRAM(PeriodicPalette);
