// Display_Dingo - Used by the event sommerhack.dk
// Based on ESPOPC -- Open Pixel Control server for ESP8266.

// The MIT License (MIT)

// Copyright (c) 2015 by bbx10node@gmail.com 
// Copyright (C) 2016 Georg Sluyterman <georg@sman.dk>
// Copyright (C) 2017 Torsten Martinsen <torsten@bullestock.net>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>

const char* ssids[] = {
    "bullestock-guest",
    "hal9k"
};
const char* password = "";

MDNSResponder mdns;
const char myDNSName[] = "displaydingo1";

WiFiUDP Udp;
#define OSCDEBUG    (0)

const int NUM_LEDS_PER_POLE = 30;
const int NUM_LEDS = NUM_LEDS_PER_POLE*2; //!!
const int UPDATES_PER_SECOND = 100;
const int PixelPin = D8;
const int BRIGHTNESS = 100; // percent

const uint8_t BeatsPerMinute = 62;

CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t startIndex = 0;
unsigned long auto_last_mode_switch = 0;
unsigned long hue_millis = 0;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void setup()
{
  FastLED.addLeds<WS2811, PixelPin, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(115200);
  Serial.println("Sommerhack LED");

  // // this resets all the neopixels to an off state
  // strip.setBrightness(255);
  // strip.begin();
  // strip.show();

  // Connect to WiFi network
  int index = 0;
  while (1)
  {
      Serial.println();
      Serial.println();
      Serial.print("Trying to connect to ");
      Serial.println(ssids[index]);

      WiFi.begin(ssids[index], password);

      int i = 0;
      bool connected = false;
      while (i < 15)
      {
          if (WiFi.status() == WL_CONNECTED)
          {
              connected = true;
              break;
          }
          delay(500);
          Serial.print(".");
          ++i;
      }
      if (connected)
      {
          Serial.println("");
          Serial.print("Connected to ");
          Serial.println(ssids[index]);
          break;
      }
      Serial.println("");
      ++index;
      if (index >= sizeof(ssids)/sizeof(const char*))
          index = 0;
  }
  
  // Print the IP address
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  if (!mdns.begin(myDNSName, WiFi.localIP()))
      Serial.println("Error setting up MDNS responder!");
  else
  {
      Serial.println("mDNS responder started");
      Serial.printf("My name is [%s]\r\n", myDNSName);
  }

  Udp.begin(7890);
  all_white();
  Serial.println("Set to all white");
  delay(1000);
  clear_all();

  const auto now = millis();
  auto_last_mode_switch = now;
  hue_millis = now;
}

WiFiClient client;

bool dirtyshow = false;

void blit_cmd(uint8_t* data, int size)
{
    if (size < sizeof(int16_t))
        return;
    auto cmdptr = (int16_t*) data;
    size -= sizeof(int16_t);
    auto pixrgb = data + sizeof(int16_t);
    int offset = *cmdptr;
    //Serial.printf("Offset: %i, Size: %i\n", offset, size / 3);
    //!! for (int i = offset; i < std::min(size / 3 + offset, NUM_LEDS); i++)
    //     strip.setPixelColor(i, GammaLUT[*pixrgb++], GammaLUT[*pixrgb++], GammaLUT[*pixrgb++]);
    dirtyshow = true;
}


uint8_t rcv[10 + 300 * 3];

const unsigned long MODE_DURATION = 10000; // ms

enum AutonomousMode
{
    OFF,
    FIRST,
    CYCLE = FIRST,
    GROWING_BARS,
    FADE,
    CHASE,
    CHASE_MULTI,
    PERIODIC_PALETTE,
    RAINBOW,
    RAINBOW_GLITTER,
    CYLON,
    BOUNCE,
    CONFETTI,
    SINELON,
    BPM,
    JUGGLE,
    FIRE,
    LAST
};

AutonomousMode autonomous_mode = AutonomousMode::FIRST;
AutonomousMode old_mode = autonomous_mode;

void clientEventUdp()
{
    int32_t packetSize = 0;
    while ((packetSize = Udp.parsePacket()))
    {
        if (autonomous_mode != AutonomousMode::OFF)
        {
            Serial.println("Switch to non-autonomous mode");
            autonomous_mode = AutonomousMode::OFF;
        }
    
        auto len = Udp.read(rcv, sizeof(rcv));
        auto cmdptr = (uint16_t*) rcv;
        auto cmd = cmdptr[0];
        if (cmd == 1111)
            blit_cmd(rcv + sizeof(int16_t), len - sizeof(int16_t)); 
    }
}

int current_led = 0;
int current_loop = 0;
bool growing = true;
uint8_t starthue = 0;

void ChangePalettePeriodically();
void FillLEDsFromPaletteColors(uint8_t colorIndex);
void SetupBlackAndWhiteStripedPalette();
void SetupPurpleAndGreenPalette();
void SetupTotallyRandomPalette();
void Fire2012();

void clear_all()
{
    memset(leds, 0, NUM_LEDS * 3);
}

void all_white()
{
    memset(leds, 255, NUM_LEDS * 3);    
    show();
}

void all_black()
{
    clear_all();
    show();
}

void show()
{
    FastLED.show();
}

const static CRGB chase_colours[] = {
    CRGB::Yellow, CRGB::Green, CRGB::HotPink, CRGB::Blue, CRGB::Red, CRGB::White
};

void fadeall()
{
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i].nscale8(250);
}

void addGlitter(fract8 chanceOfGlitter) 
{
    if (random8() < chanceOfGlitter)
        leds[random16(NUM_LEDS)] += CRGB::White;
}

#define CHECK_MODE()                    \
    if (autonomous_mode != old_mode)    \
    {                                   \
        old_mode = autonomous_mode;     \
        current_led = current_loop = 0; \
        break;                          \
    }

void runAutonomous()
{
    if (autonomous_mode == AutonomousMode::OFF)
        return;
    const auto now = millis();
    if (now - auto_last_mode_switch > MODE_DURATION)
    {
        all_white();
        auto_last_mode_switch = now;
        autonomous_mode = static_cast<AutonomousMode>(static_cast<int>(autonomous_mode)+1);
        if (autonomous_mode >= AutonomousMode::LAST)
            autonomous_mode = AutonomousMode::FIRST;
        Serial.print("Autonomous mode: ");
        Serial.println(autonomous_mode);
        delay(100);
        all_black();
    }
    delay(1);

    switch (autonomous_mode)
    {
    case AutonomousMode::CYCLE:
        // one at a time
        if (current_loop >= 3)
            current_loop = 0;
        if (current_led >= NUM_LEDS)
        {
            current_led = 0;
            ++current_loop;
        }
        clear_all();
        switch (current_loop)
        { 
        case 0: leds[current_led].r = 255; break;
        case 1: leds[current_led].g = 255; break;
        case 2: leds[current_led].b = 255; break;
        }
        ++current_led;
        break;

    case AutonomousMode::GROWING_BARS:
        // growing/receeding bars
        if (current_led >= NUM_LEDS)
        {
            current_led = 0;
            ++current_loop;
        }
        if (growing)
        {
            if (current_loop >= 3)
            {
                current_loop = 0;
                growing = false;
                break;
            }
            switch (current_loop)
            { 
            case 0: leds[current_led].r = 255; break;
            case 1: leds[current_led].g = 255; break;
            case 2: leds[current_led].b = 255; break;
            }
        }
        else
        {
            if (current_loop >= 3)
            {
                current_loop = 0;
                growing = true;
                break;
            }
            switch (current_loop)
            { 
            case 0: leds[NUM_LEDS-1-current_led].r = 0; break;
            case 1: leds[NUM_LEDS-1-current_led].g = 0; break;
            case 2: leds[NUM_LEDS-1-current_led].b = 0; break;
            }
        }
        ++current_led;
        break;

    case AutonomousMode::FADE:
        // Fade in/fade out
        for (int j = 0; j < 3; j++ )
        { 
            memset(leds, 0, NUM_LEDS * 3);
            for (int k = 0; k < 256; k++)
            {
                for (int i = 0; i < NUM_LEDS; i++ )
                    switch(j)
                    {
                    case 0: leds[i].r = k; break;
                    case 1: leds[i].g = k; break;
                    case 2: leds[i].b = k; break;
                    }
                show();
                delay(3);
                CHECK_MODE();
            }
            for (int k = 255; k >= 0; k--)
            { 
                for (int i = 0; i < NUM_LEDS; i++ )
                    switch(j) { 
                    case 0: leds[i].r = k; break;
                    case 1: leds[i].g = k; break;
                    case 2: leds[i].b = k; break;
                    }
                show();
                delay(3);
                CHECK_MODE();
            }
        }
        break;

    case AutonomousMode::CHASE:
        memset(leds, 0, NUM_LEDS * 3);
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            for (int i = 0; i < NUM_LEDS; ++i)
            {
                if (i)
                    leds[i-1] = CRGB::Black;
                leds[i] = chase_colours[c];
                show();
                delay(50);
                CHECK_MODE();
            }
            leds[NUM_LEDS-1] = CRGB::Black;
        }
        break;

    case AutonomousMode::BOUNCE:
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            memset(leds, 0, NUM_LEDS * 3);
            show();
            for (int i = 0; i < NUM_LEDS; ++i)
            {
                if (i)
                    leds[i-1] = CRGB::Black;
                leds[i] = chase_colours[c];
                show();
                delay(50);
                CHECK_MODE();
            }
            leds[NUM_LEDS-1] = CRGB::Black;
            show();
            delay(50);
            for (int i = 0; i < NUM_LEDS; ++i)
            {
                if (i)
                    leds[NUM_LEDS-i] = CRGB::Black;
                leds[NUM_LEDS-1-i] = chase_colours[c];
                show();
                delay(50);
                CHECK_MODE();
            }
            leds[NUM_LEDS-1] = CRGB::Black;
        }
        break;
    
    case AutonomousMode::CHASE_MULTI:
        memset(leds, 0, NUM_LEDS * 3);
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            const int N = 4;
            for (int j = 0; j < N; ++j)
            {
                memset(leds, 0, NUM_LEDS * 3);
                for (int i = 0; i < NUM_LEDS; ++i)
                    if (((i+j) % N) == 0)
                        leds[i] = chase_colours[c];
                show();
                delay(100);
                CHECK_MODE();
            }
            leds[NUM_LEDS-1] = CRGB::Black;
        }
        break;

    case AutonomousMode::PERIODIC_PALETTE:
        ChangePalettePeriodically();
    
        startIndex = startIndex + 1; /* motion speed */
    
        FillLEDsFromPaletteColors(startIndex);
        break;

    case AutonomousMode::RAINBOW:
        fill_rainbow(leds, NUM_LEDS, --starthue, 20);
        break;

    case AutonomousMode::RAINBOW_GLITTER:
        fill_rainbow(leds, NUM_LEDS, --starthue, 20);
        addGlitter(80);
        break;

    case AutonomousMode::CYLON:
        if (current_led >= NUM_LEDS)
            current_led = 0;
        // Set the i'th led to red 
        leds[current_led] = CHSV(starthue++, 255, 255);
        // Show the leds
        show(); 
        fadeall();
        // Wait a little bit before we loop around and do it again
        FastLED.delay(10);
        ++current_led;
        break;

    case AutonomousMode::CONFETTI:
        fadeToBlackBy(leds, NUM_LEDS, 10);
        leds[random16(NUM_LEDS)] += CHSV(starthue + random8(64), 200, 255);
        break;
    
    case AutonomousMode::SINELON:
        fadeToBlackBy(leds, NUM_LEDS, 20);
        leds[beatsin16(13, 0, NUM_LEDS)] += CHSV(starthue, 255, 192);
        break;

    case AutonomousMode::BPM:
        {
            const uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
            for (int i = 0; i < NUM_LEDS; i++)
                leds[i] = ColorFromPalette(PartyColors_p, starthue+(i*2), beat-starthue+(i*10));
        }
        break;

    case AutonomousMode::JUGGLE:
        // eight colored dots, weaving in and out of sync with each other
        {
            fadeToBlackBy(leds, NUM_LEDS, 20);
            byte dothue = 0;
            for (int i = 0; i < 8; i++)
            {
                leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
                dothue += 32;
            }
        }
        break;

    case AutonomousMode::FIRE:
        Fire2012();
        break;

    default:
        clear_all();
        break;
    }
    
    show();
    delay(1000 / UPDATES_PER_SECOND);

    if (now - hue_millis >= 20)
    {
        hue_millis = now;
        ++starthue;
    }
}


void loop()
{
    clientEventUdp();

    runAutonomous();
    
    //!! if (dirtyshow && strip.canShow())
    // {
    //     strip.show();
    //     dirtyshow = false;
    // }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;
    
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void ChangePalettePeriodically()
{
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
}

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


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
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

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

bool gReverseDirection = false;

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS-1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}
