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

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

const char* version = "0.1.0";

const char* ssids[] = {
    "Vammen Camping",
    "bullestock-guest",
    "hal9k"
};
const char* password = "";

MDNSResponder mdns;
const char myDNSName[] = "displaydingo1";

WiFiUDP Udp;

const int NUM_LEDS_PER_POLE = 30;
const int NUM_POLES_PER_STRAND = 12;
const int NUM_OF_STRANDS = 2;
const int NUM_LEDS = NUM_OF_STRANDS*NUM_POLES_PER_STRAND*NUM_LEDS_PER_POLE;
const int UPDATES_PER_SECOND = 100;
// Pin for controlling strand 1
const int PixelPin1 = D8;
// Pin for controlling strand 2
const int PixelPin2 = D7;
// Pin for controlling status LED
const int StatusPin = D6;

const int BRIGHTNESS = 100; // percent

// Blink duration when connected
const int BLINK_TICK_INTERVAL = 2000;

const uint8_t BeatsPerMinute = 62;

static CRGB leds[NUM_LEDS];
static CRGB ledsX[NUM_LEDS];
// Array of temperature readings at each simulation cell
static byte heat[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t startIndex = 0;
unsigned long auto_last_mode_switch = 0;
unsigned long hue_millis = 0;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

enum class StripMode
{
    // Use the whole strip for the animation
    WholeStrip,
    First = WholeStrip,
    // Use one pole for the animation, and copy to remaining poles
    OnePoleCopy,
    // Use one pole for the animation, and shift-copy to remaining poles
    OnePoleShiftCopy,
    Last
};

StripMode strip_mode = StripMode::WholeStrip;

int effective_leds = 0;

void set_strip_mode(StripMode mode)
{
    switch (mode)
    {
    case StripMode::WholeStrip:
        effective_leds = NUM_LEDS;
        break;

    case StripMode::OnePoleCopy:
    case StripMode::OnePoleShiftCopy:
        effective_leds = NUM_LEDS_PER_POLE;
        break;

    default:
        return;
    }
    strip_mode = mode;
}

void clear_all();
void show();
void random_burst();
void flicker();
void pulse_one_color_all();
void pulse_one_color_all_rev();
void radiation();
void color_loop_vardelay();
void sin_bright_wave();
void random_color_pop();
void ems_lightsSTROBE();
void rgb_propeller();
void kitt();
void matrix();

void setup()
{
    pinMode(StatusPin, OUTPUT);
    digitalWrite(StatusPin, 0);
    
    set_strip_mode(StripMode::WholeStrip);
    
    if (NUM_OF_STRANDS > 1)
    {
        FastLED.addLeds<WS2811, PixelPin1, GRB>(leds, 0, NUM_LEDS/2).setCorrection(TypicalLEDStrip);
        FastLED.addLeds<WS2811, PixelPin2, GRB>(leds, NUM_LEDS/2, NUM_LEDS/2).setCorrection(TypicalLEDStrip);
    }
    else
        FastLED.addLeds<WS2811, PixelPin1, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    
    FastLED.setBrightness(BRIGHTNESS);

    delay(1000);
    Serial.begin(115200);
    Serial.print("\r\nSommerhack LED ");
    Serial.println(version);

    Serial.print("Poles per strand: ");
    Serial.println(NUM_POLES_PER_STRAND);
    
    // Connect to WiFi network
    WiFi.mode(WIFI_STA);
    int index = 0;
    while (1)
    {
        Serial.println();
        Serial.println();
        Serial.print("Trying to connect to ");
        Serial.println(ssids[index]);

        WiFi.begin(ssids[index], password);
        digitalWrite(StatusPin, 0);

        int i = 0;
        bool connected = false;
        while (i < 15)
        {
            digitalWrite(StatusPin, 1);
            delay(250);
            if (WiFi.status() == WL_CONNECTED)
            {
                connected = true;
                break;
            }
            digitalWrite(StatusPin, 0);
            delay(250);
            Serial.print(".");
            ++i;
        }
        if (connected)
        {
            for (int i = 0; i < 10; ++i)
            {
                delay(80);
                digitalWrite(StatusPin, 1);
                delay(80);
                digitalWrite(StatusPin, 0);
            }
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
    clear_all();
    show();
    
    const auto now = millis();
    auto_last_mode_switch = now;
    hue_millis = now;
}

WiFiClient client;

bool dirtyshow = false;

enum class AutonomousMode
{
    OFF = -1,
    CYCLE,
    FIRST = CYCLE,
    GROWING_BARS,
    CHASE,
    PERIODIC_PALETTE,
    RAINBOW,
    CYLON,
    BOUNCE,
    CONFETTI,
    SINELON,
    BPM,
    JUGGLE,
    FIRE,
    RANDOM_BURST,
    FLICKER,
    PULSE,
    RADIATION,
    RAINBOW_GLITTER,
    COLOR_LOOP,
    SIN_BRIGHT,
    RANDOM_POP,
    PULSE_REV,
    STROBE,
    PROPELLER,
    KITT,
    CHASE_MULTI,
    MATRIX,
    LAST
};

const char* mode_names[] =
{
    "", // off
    "Cycle",
    "Growing bars",
    "Chase",
    "Periodic palette",
    "Rainbow",
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
    "Radiation",
    "Rainbow glitter",
    "Color loop",
    "Sin bright",
    "Random pop",
    "Pulse rev",
    "Strobe",
    "Propeller",
    "Kitt",
    "Chase multi",
    "Matrix"
};

AutonomousMode autonomous_mode = AutonomousMode::RAINBOW_GLITTER;
bool auto_mode_switch = true;
uint16_t autonomous_speed = 1000/UPDATES_PER_SECOND;

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
    if (*cmdptr >= static_cast<uint8_t>(AutonomousMode::LAST))
        return;
    auto_mode_switch = false;
    autonomous_mode = static_cast<AutonomousMode>(*cmdptr);
    set_strip_mode(strip_mode);
    Serial.printf("Set mode %d\r\n", autonomous_mode);
}

void parse_strip_mode(uint8_t* data, int size)
{
    if (size < sizeof(uint8_t))
        return;
    auto cmdptr = (uint8_t*) data;
    set_strip_mode(static_cast<StripMode>(*cmdptr));
    Serial.printf("Set strip mode %d\r\n", strip_mode);
}

void parse_speed(uint8_t* data, int size)
{
    if (size < sizeof(uint8_t))
        return;
    auto cmdptr = (uint16_t*) data;
    autonomous_speed = *cmdptr;
    if (autonomous_speed < 10)
        autonomous_speed = 10;
    Serial.printf("Set speed %d\r\n", autonomous_speed);
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
            break;
            
        case 1113:
            // Set autonomous strip mode (1 byte)
            parse_strip_mode(rcv + sizeof(int16_t), len - sizeof(int16_t));
            break;
            
        case 1114:
            // Set autonomous speed (1 byte)
            parse_speed(rcv + sizeof(int16_t), len - sizeof(int16_t));
            break;
            
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
    memset(leds, 0, effective_leds * 3);
}

void show()
{
    switch (strip_mode)
    {
    case StripMode::WholeStrip:
        break;

    case StripMode::OnePoleCopy:
        for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
            for (int j = 1; j < NUM_POLES_PER_STRAND*NUM_OF_STRANDS; ++j)
                leds[j*NUM_LEDS_PER_POLE+i] = leds[i];
        break;

    case StripMode::OnePoleShiftCopy:
        for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
            for (int j = 1; j < NUM_POLES_PER_STRAND*NUM_OF_STRANDS; ++j)
                leds[j*NUM_LEDS_PER_POLE+(i+j*3)%NUM_LEDS_PER_POLE] = leds[i];
        break;
    }

    // Mirror strand 1
    if (NUM_OF_STRANDS > 1)
        for (int p = 0; p < NUM_POLES_PER_STRAND/2; ++p)
        {
            CRGB temp[NUM_LEDS_PER_POLE];
            for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
                temp[i] = leds[p*NUM_LEDS_PER_POLE+i];
            for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
                leds[p*NUM_LEDS_PER_POLE+i] = leds[(NUM_POLES_PER_STRAND-1-p)*NUM_LEDS_PER_POLE+i];
            for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
                leds[(NUM_POLES_PER_STRAND-1-p)*NUM_LEDS_PER_POLE+i] = temp[i];
        }
    
    FastLED.show();
}

const static CRGB chase_colours[] = {
    CRGB::Yellow, CRGB::Green, CRGB::HotPink, CRGB::Blue, CRGB::Red, CRGB::White
};

void fadeall()
{
    for (int i = 0; i < effective_leds; i++)
        leds[i].nscale8(250);
}

void addGlitter(fract8 chanceOfGlitter) 
{
    if (random8() < chanceOfGlitter)
        leds[random16(effective_leds)] += CRGB::White;
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
            for (int i = 0; i < effective_leds; i++)
                leds[i].nscale8(200);
            show();
            delay(100);
        }
        auto_last_mode_switch = now;
        autonomous_mode = static_cast<AutonomousMode>(static_cast<int>(autonomous_mode)+1);
        if (autonomous_mode >= AutonomousMode::LAST)
        {
            autonomous_mode = AutonomousMode::FIRST;
            auto new_strip_mode = static_cast<StripMode>(static_cast<int>(strip_mode)+1);
            if (new_strip_mode >= StripMode::Last)
                new_strip_mode = StripMode::First;
            set_strip_mode(new_strip_mode);
        }
        Serial.print("Autonomous mode ");
        Serial.print(static_cast<int>(autonomous_mode));
        Serial.print(": ");
        Serial.println(mode_names[static_cast<int>(autonomous_mode)]);
    }

    switch (autonomous_mode)
    {
    case AutonomousMode::CYCLE:        // one at a time
        if (current_loop >= 3)
            current_loop = 0;
        if (current_led >= effective_leds)
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
        if (current_led >= effective_leds)
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
            case 0: leds[effective_leds-1-current_led].r = 0; break;
            case 1: leds[effective_leds-1-current_led].g = 0; break;
            case 2: leds[effective_leds-1-current_led].b = 0; break;
            }
        }
        ++current_led;
        break;

    case AutonomousMode::CHASE:
        memset(leds, 0, effective_leds * 3);
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            for (int i = 0; i < effective_leds; ++i)
            {
                if (i)
                    leds[i-1] = CRGB::Black;
                leds[i] = chase_colours[c];
                show();
                delay(50);
            }
            leds[effective_leds-1] = CRGB::Black;
        }
        break;

    case AutonomousMode::BOUNCE:
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            memset(leds, 0, effective_leds * 3);
            for (int i = 0; i < effective_leds; ++i)
            {
                if (i)
                    leds[i-1] = CRGB::Black;
                leds[i] = chase_colours[c];
                show();
                delay(50);
            }
            leds[effective_leds-1] = CRGB::Black;
            show();
            delay(50);
            for (int i = 0; i < effective_leds; ++i)
            {
                if (i)
                    leds[effective_leds-i] = CRGB::Black;
                leds[effective_leds-1-i] = chase_colours[c];
                show();
                delay(50);
            }
            leds[effective_leds-1] = CRGB::Black;
        }
        break;
    
    case AutonomousMode::CHASE_MULTI:
        memset(leds, 0, effective_leds * 3);
        for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
        {
            const int N = 4;
            for (int j = 0; j < N; ++j)
            {
                memset(leds, 0, effective_leds * 3);
                for (int i = 0; i < effective_leds; ++i)
                    if (((i+j) % N) == 0)
                        leds[i] = chase_colours[c];
                show();
                delay(100);
            }
            leds[effective_leds-1] = CRGB::Black;
        }
        break;

    case AutonomousMode::PERIODIC_PALETTE:
        ChangePalettePeriodically();
    
        startIndex = startIndex + 1; /* motion speed */
    
        FillLEDsFromPaletteColors(startIndex);
        break;

    case AutonomousMode::RAINBOW:
        fill_rainbow(leds, effective_leds, --starthue, 20);
        break;

    case AutonomousMode::RAINBOW_GLITTER:
        fill_rainbow(leds, effective_leds, --starthue, 20);
        addGlitter(80);
        break;

    case AutonomousMode::CYLON:
        if (current_led >= effective_leds)
            current_led = 0;
        // Set the i'th led to red 
        leds[current_led] = CHSV(starthue++, 255, 255);
        ++current_led;
        break;

    case AutonomousMode::CONFETTI:
        fadeToBlackBy(leds, effective_leds, 10);
        leds[random16(effective_leds)] += CHSV(starthue + random8(64), 200, 255);
        break;
    
    case AutonomousMode::SINELON:
        fadeToBlackBy(leds, effective_leds, 20);
        leds[beatsin16(13, 0, effective_leds)] += CHSV(starthue, 255, 192);
        break;

    case AutonomousMode::BPM:
        {
            const uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
            for (int i = 0; i < effective_leds; i++)
                leds[i] = ColorFromPalette(PartyColors_p, starthue+(i*2), beat-starthue+(i*10));
        }
        break;

    case AutonomousMode::JUGGLE:
        // eight colored dots, weaving in and out of sync with each other
        {
            fadeToBlackBy(leds, effective_leds, 20);
            byte dothue = 0;
            for (int i = 0; i < 8; i++)
            {
                leds[beatsin16(i+7, 0, effective_leds)] |= CHSV(dothue, 200, 255);
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
    delay(autonomous_speed);

    if (now - hue_millis >= 20)
    {
        hue_millis = now;
        ++starthue;
    }
}

unsigned long blink_tick = 0;
bool status_led_on = false;

void loop()
{
    clientEventUdp();

    runAutonomous();
    
    if (dirtyshow)
    {
        FastLED.show();
        dirtyshow = false;
    }

    const auto ticks = millis();
    if (ticks - blink_tick > BLINK_TICK_INTERVAL)
    {
        blink_tick = ticks;
        status_led_on = !status_led_on;
        digitalWrite(StatusPin, status_led_on);
    }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;
    
  for (int i = 0; i < effective_leds; i++) {
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
// This simulation scales it self a bit depending on effective_leds; it should look
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
    // Step 1.  Cool down every cell a little
    for (int i = 0; i < effective_leds; i++) {
        heat[i] = qsub8(heat[i],  random8(0, ((COOLING * 10) / effective_leds) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k= effective_leds - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if(random8() < SPARKING) {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160,255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < effective_leds; j++) {
        CRGB color = HeatColor(heat[j]);
        int pixelnumber;
        if(gReverseDirection) {
            pixelnumber = (effective_leds-1) - j;
        } else {
            pixelnumber = j;
        }
        leds[pixelnumber] = color;
    }
}

//----------

int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;           //-SET SINGLE LED VAR
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

//---LED FX VARS
int idex = 0;                //-LED INDEX (0 to effective_leds-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR

void copy_led_array()
{
    for (int i = 0; i < effective_leds; i++)
        ledsX[i] = leds[i];
}

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
    for (int i = 0 ; i < effective_leds; i++)
        leds[i].setRGB(cred, cgrn, cblu);
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
    for (int i = 0 ; i < effective_leds; i++)
      leds[i] = CHSV(ahue, thissat, 255);
}

void random_burst()
{                         //-m4-RANDOM INDEX/COLOR
    idex = random(0, effective_leds);
    ihue = random(0, 255);  
    leds[idex] = CHSV(ihue, thissat, 255);
}

void flicker()
{                          //-m9-FLICKER EFFECT
    int random_bright = random(0,255);
    int random_delay = random(10,100);
    int random_bool = random(0,random_bright);
    if (random_bool < 10)
        for (int i = 0 ; i < effective_leds; i++)
            leds[i] = CHSV(thishue, thissat, random_bright);
}

void pulse_one_color_all()
{
    //-m10-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR 
    if (bouncedirection == 0) {
        ibright++;
        if (ibright >= 255)
            bouncedirection = 1;
    }
    if (bouncedirection == 1)
    {
        ibright = ibright - 1;
        if (ibright <= 1)
            bouncedirection = 0;
    }  
    for (int idex = 0 ; idex < effective_leds; idex++)
        leds[idex] = CHSV(thishue, thissat, ibright);
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
    for (int idex = 0 ; idex < effective_leds; idex++) {
        leds[idex] = CHSV(thishue, isat, 255);
    }
}

void random_red() {                       //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED  
    int temprand;
    for (int i = 0; i < effective_leds; i++) {
        temprand = random(0,100);
        if (temprand > 50) {leds[i].r = 255;}
        if (temprand <= 50) {leds[i].r = 0;}
        leds[i].b = 0; leds[i].g = 0;
    }
}

void radiation() {                   //-m16-SORT OF RADIATION SYMBOLISH- 
    int N3  = int(effective_leds/3);
    int N6  = int(effective_leds/6);  
    int N12 = int(effective_leds/12);  
    for (int i = 0; i < N6; i++) {     //-HACKY, I KNOW...
        tcount = tcount + .02;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);    
        int j0 = (i + effective_leds - N12) % effective_leds;
        int j1 = (j0+N3) % effective_leds;
        int j2 = (j1+N3) % effective_leds;    
        leds[j0] = CHSV(thishue, thissat, ibright);
        leds[j1] = CHSV(thishue, thissat, ibright);
        leds[j2] = CHSV(thishue, thissat, ibright);    
    }    
}

void color_loop_vardelay() {                    //-m17-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
    idex++;
    if (idex > effective_leds) {idex = 0;}
    for (int i = 0; i < effective_leds; i++) {
        if (i == idex) {
            leds[i] = CHSV(0, thissat, 255);
        }
        else {
            leds[i].r = 0; leds[i].g = 0; leds[i].b = 0;
        }
    }
}

void sin_bright_wave() {        //-m19-BRIGHTNESS SINE WAVE
    for (int i = 0; i < effective_leds; i++) {
        tcount = tcount + .1;
        if (tcount > 3.14)
            tcount = 0.0;
        ibright = int(sin(tcount)*255);
        leds[i] = CHSV(thishue, thissat, ibright);
    }
}

void random_color_pop() {                         //-m25-RANDOM COLOR POP
    idex = random(0, effective_leds);
    ihue = random(0, 255);
    one_color_all(0, 0, 0);
    leds[idex] = CHSV(ihue, thissat, 255);
}

void ems_lightsSTROBE() {                  //-m26-EMERGENCY LIGHTS (STROBE LEFT/RIGHT)
    int thishue = 0;
    int thathue = (thishue + 160) % 255;
    int thisdelay = 25;
    const int TOP_INDEX = int(effective_leds/2);
    for (int x = 0 ; x < 5; x++) {
        for (int i = 0 ; i < TOP_INDEX; i++)
            leds[i] = CHSV(thishue, thissat, 255);
        show(); delay(thisdelay); 
        one_color_all(0, 0, 0);
        show(); delay(thisdelay);
    }
    for (int x = 0 ; x < 5; x++) {
        for (int i = TOP_INDEX ; i < effective_leds; i++)
            leds[i] = CHSV(thathue, thissat, 255);
        show(); delay(thisdelay);
        one_color_all(0, 0, 0);
        show(); delay(thisdelay);
    }
}

void rgb_propeller()
{                           //-m27-RGB PROPELLER 
    idex++;
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
}

void kitt()
{
    int thisdelay = 100;
    const int TOP_INDEX = int(effective_leds/2);
    int rand = random(0, TOP_INDEX);
    for (int i = 0; i < rand; i++) {
        leds[TOP_INDEX+i] = CHSV(thishue, thissat, 255);
        leds[TOP_INDEX-i] = CHSV(thishue, thissat, 255);
        show();
        delay(thisdelay/rand);
    }
    for (int i = rand; i > 0; i--) {
        leds[TOP_INDEX+i] = CHSV(thishue, thissat, 0);
        leds[TOP_INDEX-i] = CHSV(thishue, thissat, 0);
        show();
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
    for (int i = 1; i < effective_leds; i++) {
        leds[i] = ledsX[i-1];
    }
}
