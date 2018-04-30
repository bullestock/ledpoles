// Display_Dingo - Used by the event sommerhack.dk

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

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include "SSD1306.h" 

#include "neomatrix.hpp"
#include "stripmode.hpp"

const char* version = "0.2.0";

const char* ssids[] = {
    "Vammen Camping",
    "bullestock-guest",
    "hal9k"
};
const char* password = "";

MDNSResponder mdns;
const char myDNSName[] = "displaydingo2";

WiFiUDP Udp;

SSD1306 display(0x3c, 21, 22, GEOMETRY_128_32);

const int NUM_LEDS_PER_POLE = 30;
const int NUM_POLES_PER_STRAND = 1;
const int NUM_OF_STRANDS = 2;
const int NUM_LEDS = NUM_OF_STRANDS*NUM_POLES_PER_STRAND*NUM_LEDS_PER_POLE;
// Pin for controlling strand 1
const int PixelPin1 = 2;
// Pin for controlling strand 2
const int PixelPin2 = 4;
// Pin for controlling status LED
const int StatusPin = 5;

const int BRIGHTNESS = 100; // percent

// Blink duration when connected
const int BLINK_TICK_INTERVAL = 2000;

// Main animation buffer
CRGB* leds = nullptr;

// Buffer for swapping strand1 before showing
CRGB* strand1 = nullptr;

// The number of LEDs available to the animation algorithms
int effective_leds = 0;

static StripMode strip_mode = StripMode::WholeStrip;

StripMode get_strip_mode()
{
    return strip_mode;
}

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

void setup()
{
    leds = new CRGB[NUM_LEDS];
    
    pinMode(StatusPin, OUTPUT);
    digitalWrite(StatusPin, 0);
    
    set_strip_mode(StripMode::WholeStrip);
    
    if (NUM_OF_STRANDS > 1)
    {
        // Strand 1 must be swapped before display, so we need a separate display buffer
        strand1 = new CRGB[NUM_LEDS/2];
        FastLED.addLeds<WS2811, PixelPin1, GRB>(strand1, NUM_LEDS/2).setCorrection(TypicalLEDStrip);
        FastLED.addLeds<WS2811, PixelPin2, GRB>(leds, NUM_LEDS/2, NUM_LEDS/2).setCorrection(TypicalLEDStrip);
    }
    else
        FastLED.addLeds<WS2811, PixelPin1, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    
    FastLED.setBrightness(BRIGHTNESS);

    delay(1000);
    Serial.begin(115200);
    Serial.print("\r\nSommerhack LED ");
    Serial.println(version);
    display.init();
    display.drawString(0, 0, version);
    display.display();
    delay(2000);

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

        WiFi.disconnect();
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
            display.clear();
            display.drawString(0, 0, "Connected to");
            display.drawString(0, 16, ssids[index]);
            display.display();
        
            display.display();
            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(ssids[index]);
            delay(1000);
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
    if (!mdns.begin(myDNSName))
    {
        Serial.println("Error setting up mDNS responder!");
        display.clear();
        display.drawString(0, 0, "mDNS error");
        display.display();
    }
    else
    {
        Serial.println("mDNS responder started");
        Serial.printf("My name is [%s]\r\n", myDNSName);
        display.clear();
        display.drawString(0, 0, myDNSName);
        display.display();
    }

    Udp.begin(7890);
    
    clear_all();
    show();
    
    neomatrix_init();
}

WiFiClient client;

bool dirtyshow = false;

extern bool run_autonomously;

void parse_pixel_data(uint8_t* data, int size)
{
    if (size < sizeof(int16_t))
        return;

    if (run_autonomously)
    {
        run_autonomously = false;
        Serial.println("Switch to non-autonomous mode");
        FastLED.setBrightness(255);
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
    data[size] = 0;
    auto mode = (const char*) data;
    Serial.printf("Set mode %s\r\n", mode);
    neomatrix_change_program(mode);
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
    auto autonomous_speed = *data;
    if (autonomous_speed > 50)
        autonomous_speed = 50;
    if (autonomous_speed <= 0)
        autonomous_speed = 1;
    Serial.printf("Set speed %d\r\n", autonomous_speed);
    neomatrix_set_speed(autonomous_speed);
}

void parse_brightness(uint8_t* data, int size)
{
    if (size < sizeof(uint8_t))
        return;
    auto brightness = *data;
    if (brightness < 10)
        brightness = 10;
    Serial.printf("Set brightness %d\r\n", brightness);
    neomatrix_set_brightness(brightness);
}

void parse_nightmode(uint8_t* data, int size)
{
    if (size < sizeof(uint8_t))
        return;
    auto nightmode = *data;
    if (nightmode > 1)
        nightmode = 1;
    Serial.printf("Set night mode %d\r\n", nightmode);
    neomatrix_set_nightmode(nightmode);
}

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
            // Set autonomous mode (string)
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
            
        case 1115:
            // Start autonomous mode switch (no arguments)
            neomatrix_start_autorun();
            break;

        case 1116:
            // Set brightness (1 byte)
            parse_brightness(rcv + sizeof(int16_t), len - sizeof(int16_t));
            break;

        case 1117:
            // Night mode (1 byte)
            parse_nightmode(rcv + sizeof(int16_t), len - sizeof(int16_t));
            break;
            
        default:
            break;
        }
            
    }
}

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
        for (int p = 0; p < NUM_POLES_PER_STRAND; ++p)
            for (int i = 0; i < NUM_LEDS_PER_POLE; ++i)
                strand1[p*NUM_LEDS_PER_POLE+i] = leds[(NUM_POLES_PER_STRAND-1-p)*NUM_LEDS_PER_POLE+i];
    
    FastLED.show();
}

unsigned long blink_tick = 0;
bool status_led_on = false;

void loop()
{
    clientEventUdp();

    neomatrix_run();
    
    if (dirtyshow)
    {
        show();
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
