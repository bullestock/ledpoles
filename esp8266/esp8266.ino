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

const int NUM_LEDS_PER_POLE = 30;
const int NUM_POLES = 10; // ?
const int NUM_LEDS = NUM_LEDS_PER_POLE*NUM_POLES;
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
      Serial.println("Error setting up mDNS responder!");
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

enum AutonomousMode
{
    OFF,
    CYCLE,
    FIRST = CYCLE,
    GROWING_BARS,
    FADE,
    CHASE,
    CHASE_MULTI,    // 5
    PERIODIC_PALETTE,
    RAINBOW,
    RAINBOW_GLITTER,
    CYLON,
    BOUNCE,         // 10
    CONFETTI,
    SINELON,
    BPM,
    JUGGLE,
    FIRE,           // 15
    RANDOM_BURST,
    FLICKER,
    PULSE,
    PULSE_REV,
    RADIATION,
    COLOR_LOOP,
    SIN_BRIGHT,
    RANDOM_POP,
    STROBE,
    PROPELLER,
    KITT,
    MATRIX,
    LAST
};

const char* mode_names[] =
{
    "", // off
    "Cycle",
    "Growing bars",
    "Fade",
    "Chase",
    "Chase multi",
    "Periodic palette",
    "Rainbow",
    "Rainbow glitter",
    "Cylon",
    "Bounce",
    "Confetti",
    "Sinelon",
    "Bpm",
    "Juggle",
    "Fire",
    "Random burst",
    "Flicker",
    "Pulse",
    "Pulse rev",
    "Radiation",
    "Color loop",
    "Sin bright",
    "Random pop",
    "Strobe",
    "Propeller",
    "Kitt",
    "Matrix",
    "Rainbow loop"
};

AutonomousMode autonomous_mode = AutonomousMode::FIRST;
bool auto_mode_switch = true;

void parse_pixel_data(uint8_t* data, int size)
{
    if (size < sizeof(int16_t))
        return;
    if (autonomous_mode != AutonomousMode::OFF)
    {
        autonomous_mode = AutonomousMode::OFF;
        Serial.println("Switch to non-autonomous mode");
        delay(10);
    }
    
    auto cmdptr = (int16_t*) data;
    size -= sizeof(int16_t);
    auto pixrgb = data + sizeof(int16_t);
    auto nof_leds = size/3;
    int offset = *cmdptr;
    //Serial.printf("Offset: %i, Size: %i\n", offset, size / 3);
    for (int i = offset; i < std::min(offset + nof_leds, NUM_LEDS); ++i)
        leds[i] = CRGB(*pixrgb++, *pixrgb++, *pixrgb++);
    dirtyshow = true;
}

void parse_mode(uint8_t* data, int size)
{
    if (size < sizeof(uint8_t))
        return;
    auto cmdptr = (uint8_t*) data;
    auto_mode_switch = false;
    autonomous_mode = static_cast<AutonomousMode>(*cmdptr);
    Serial.printf("Set mode %d\n", autonomous_mode);
}


const unsigned long MODE_DURATION = 60000; // ms

AutonomousMode old_mode = autonomous_mode;

void clientEventUdp()
{
    int32_t packetSize = 0;
    while ((packetSize = Udp.parsePacket()))
    {
        uint8_t rcv[10 + NUM_LEDS * 3];
        auto len = Udp.read(rcv, sizeof(rcv));
        auto cmdptr = (uint16_t*) rcv;
        auto cmd = cmdptr[0];
        switch (cmd)
        {
        case 1111:
            // Pixel data
            parse_pixel_data(rcv + sizeof(int16_t), len - sizeof(int16_t));
            break;

        case 1112:
            // Set autonomous mode (1 byte)
            parse_mode(rcv + sizeof(int16_t), len - sizeof(int16_t));

        default:
            break;
        }
            
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
    if (auto_mode_switch && (now - auto_last_mode_switch > MODE_DURATION))
    {
        for (int j = 0; j < 10; ++j)
        {
            for (int i = 0; i < NUM_LEDS; i++)
                leds[i].nscale8(200);
            show();
            delay(100);
        }
        auto_last_mode_switch = now;
        autonomous_mode = static_cast<AutonomousMode>(static_cast<int>(autonomous_mode)+1);
        if (autonomous_mode >= AutonomousMode::LAST)
            autonomous_mode = AutonomousMode::FIRST;
        Serial.print("Autonomous mode ");
        Serial.print(autonomous_mode);
        Serial.print(": ");
        Serial.println(mode_names[autonomous_mode]);
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

    case AutonomousMode::RANDOM_BURST:
        random_burst();
        break;
        
    case AutonomousMode::FLICKER:
        flicker();
        break;
        
    case AutonomousMode::PULSE:
        pulse_one_color_all();
        break;
        
    case AutonomousMode::PULSE_REV:
        pulse_one_color_all_rev();
        break;
        
    case AutonomousMode::RADIATION:
        radiation();
        break;
        
    case AutonomousMode::COLOR_LOOP:
        color_loop_vardelay();
        break;
        
    case AutonomousMode::SIN_BRIGHT:
        sin_bright_wave();
        break;
        
    case AutonomousMode::RANDOM_POP:
        random_color_pop();
        break;
        
    case AutonomousMode::STROBE:
        ems_lightsSTROBE();
        break;
        
    case AutonomousMode::PROPELLER:
        rgb_propeller();
        break;
        
    case AutonomousMode::KITT:
        kitt();
        break;
        
    case AutonomousMode::MATRIX:
        matrix();
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
    
    if (dirtyshow)
    {
        FastLED.show();
        dirtyshow = false;
    }
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

//----------

int BOTTOM_INDEX = 0;
int TOP_INDEX = int(NUM_LEDS/2);
int EVENODD = NUM_LEDS%2;
int ledsX[NUM_LEDS][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;           //-SET SINGLE LED VAR
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

//---LED FX VARS
int idex = 0;                //-LED INDEX (0 to NUM_LEDS-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR

void copy_led_array(){
  for(int i = 0; i < NUM_LEDS; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }  
}

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      leds[i].setRGB( cred, cgrn, cblu);
    }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      leds[i] = CHSV(ahue, thissat, 255);
    }
}

void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  idex = random(0, NUM_LEDS);
  ihue = random(0, 255);  
  leds[idex] = CHSV(ihue, thissat, 255);
}

void flicker() {                          //-m9-FLICKER EFFECT
  int random_bright = random(0,255);
  int random_delay = random(10,100);
  int random_bool = random(0,random_bright);
  if (random_bool < 10) {
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
      leds[i] = CHSV(thishue, thissat, random_bright);
    }
  }
}

void pulse_one_color_all() {              //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR 
  if (bouncedirection == 0) {
    ibright++;
    if (ibright >= 255) {bouncedirection = 1;}
  }
  if (bouncedirection == 1) {
    ibright = ibright - 1;
    if (ibright <= 1) {bouncedirection = 0;}         
  }  
    for(int idex = 0 ; idex < NUM_LEDS; idex++ ) {
      leds[idex] = CHSV(thishue, thissat, ibright);
    }
}

void pulse_one_color_all_rev() {           //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR 
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {bouncedirection = 1;}
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {bouncedirection = 0;}         
  }  
    for(int idex = 0 ; idex < NUM_LEDS; idex++ ) {
      leds[idex] = CHSV(thishue, isat, 255);
    }
}

void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED  
  int temprand;
  for(int i = 0; i < NUM_LEDS; i++ ) {
    temprand = random(0,100);
    if (temprand > 50) {leds[i].r = 255;}
    if (temprand <= 50) {leds[i].r = 0;}
    leds[i].b = 0; leds[i].g = 0;
  }
}

void radiation() {                   //-m16-SORT OF RADIATION SYMBOLISH- 
  int N3  = int(NUM_LEDS/3);
  int N6  = int(NUM_LEDS/6);  
  int N12 = int(NUM_LEDS/12);  
  for(int i = 0; i < N6; i++ ) {     //-HACKY, I KNOW...
    tcount = tcount + .02;
    if (tcount > 3.14) {tcount = 0.0;}
    ibright = int(sin(tcount)*255);    
    int j0 = (i + NUM_LEDS - N12) % NUM_LEDS;
    int j1 = (j0+N3) % NUM_LEDS;
    int j2 = (j1+N3) % NUM_LEDS;    
    leds[j0] = CHSV(thishue, thissat, ibright);
    leds[j1] = CHSV(thishue, thissat, ibright);
    leds[j2] = CHSV(thishue, thissat, ibright);    
  }    
}

void color_loop_vardelay() {                    //-m17-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
  idex++;
  if (idex > NUM_LEDS) {idex = 0;}
  for(int i = 0; i < NUM_LEDS; i++ ) {
    if (i == idex) {
      leds[i] = CHSV(0, thissat, 255);
    }
    else {
      leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
    }
  }
}

void sin_bright_wave() {        //-m19-BRIGHTNESS SINE WAVE
  for(int i = 0; i < NUM_LEDS; i++ ) {
    tcount = tcount + .1;
    if (tcount > 3.14) {tcount = 0.0;}
    ibright = int(sin(tcount)*255);
    leds[i] = CHSV(thishue, thissat, ibright);
  }
}

void random_color_pop() {                         //-m25-RANDOM COLOR POP
  idex = random(0, NUM_LEDS);
  ihue = random(0, 255);
  one_color_all(0, 0, 0);
  leds[idex] = CHSV(ihue, thissat, 255);
}

void ems_lightsSTROBE() {                  //-m26-EMERGENCY LIGHTS (STROBE LEFT/RIGHT)
  int thishue = 0;
  int thathue = (thishue + 160) % 255;
  int thisdelay = 25;
  for(int x = 0 ; x < 5; x++ ) {
    for(int i = 0 ; i < TOP_INDEX; i++ ) {
        leds[i] = CHSV(thishue, thissat, 255);
    }
    show(); delay(thisdelay); 
    one_color_all(0, 0, 0);
    show(); delay(thisdelay);
  }
  for(int x = 0 ; x < 5; x++ ) {
    for(int i = TOP_INDEX ; i < NUM_LEDS; i++ ) {
        leds[i] = CHSV(thathue, thissat, 255);
    }
    show(); delay(thisdelay);
    one_color_all(0, 0, 0);
    show(); delay(thisdelay);
  }
}

void rgb_propeller() {                           //-m27-RGB PROPELLER 
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(NUM_LEDS/3);
  int N6  = int(NUM_LEDS/6);  
  int N12 = int(NUM_LEDS/12);  
  for(int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + NUM_LEDS - N12) % NUM_LEDS;
    int j1 = (j0+N3) % NUM_LEDS;
    int j2 = (j1+N3) % NUM_LEDS;    
    leds[j0] = CHSV(thishue, thissat, 255);
    leds[j1] = CHSV(ghue, thissat, 255);
    leds[j2] = CHSV(bhue, thissat, 255);    
  }
}

void kitt()
{
  int thisdelay = 100;
  int rand = random(0, TOP_INDEX);
  for(int i = 0; i < rand; i++ ) {
    leds[TOP_INDEX+i] = CHSV(thishue, thissat, 255);
    leds[TOP_INDEX-i] = CHSV(thishue, thissat, 255);
    LEDS.show();
    delay(thisdelay/rand);
  }
  for(int i = rand; i > 0; i-- ) {
    leds[TOP_INDEX+i] = CHSV(thishue, thissat, 0);
    leds[TOP_INDEX-i] = CHSV(thishue, thissat, 0);
    LEDS.show();
    delay(thisdelay/rand);
  }  
}

void matrix()
{
  int rand = random(0, 100);
  if (rand > 90) {
    leds[0] = CHSV(thishue, thissat, 255);
  }
  else {leds[0] = CHSV(thishue, thissat, 0);}
  copy_led_array();
    for(int i = 1; i < NUM_LEDS; i++ ) {
    leds[i].r = ledsX[i-1][0];
    leds[i].g = ledsX[i-1][1];
    leds[i].b = ledsX[i-1][2];    
  }
}

/*
void change_mode(int newmode){
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0,0,0); LEDS.show(); break;   //---ALL OFF
    case 1: one_color_all(255,255,255); LEDS.show(); break;   //---ALL ON
    case 2: thisdelay = 20; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 20; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 40; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 160; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 12: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 13: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    //---PLACEHOLDER FOR COLOR LOOP VAR DELAY VARS
    case 19: thisdelay = 35; thishue = 180; break;      //---SIN WAVE BRIGHTNESS
    case 20: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 21: thisdelay = 100; thishue = 180; break;     //---QUADRATIC BRIGHTNESS CURVE
    //---PLACEHOLDER FOR FLAME VARS
    case 23: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 25: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 26: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 27: thisdelay = 25; thishue = 0; break;        //---RGB PROPELLER
    case 28: thisdelay = 100; thishue = 0; break;       //---KITT
    case 29: thisdelay = 50; thishue = 95; break;       //---MATRIX RAIN
    case 88: thisdelay = 5; break;                      //---NEW RAINBOW LOOP
    case 101: one_color_all(255,0,0); LEDS.show(); break;   //---ALL RED
    case 102: one_color_all(0,255,0); LEDS.show(); break;   //---ALL GREEN
    case 103: one_color_all(0,0,255); LEDS.show(); break;   //---ALL BLUE
    case 104: one_color_all(255,255,0); LEDS.show(); break;   //---ALL COLOR X
    case 105: one_color_all(0,255,255); LEDS.show(); break;   //---ALL COLOR Y
    case 106: one_color_all(255,0,255); LEDS.show(); break;   //---ALL COLOR Z
  }
  bouncedirection = 0;
  one_color_all(0,0,0);
  ledMode = newmode;
}
*/

