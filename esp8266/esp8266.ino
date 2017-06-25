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

const int PixelCount = 300;
const int PixelPin = D8;
#define BRIGHTNESS 100

auto strip = Adafruit_NeoPixel(PixelCount, PixelPin, NEO_GRB + NEO_KHZ800);

// Gamma correction 2.2 look up table
uint8_t GammaLUT[256];

void fillGammaLUT(float gamma)
{
    for (int i = 0; i < 256; i++)
    {
        float intensity = (float)i / 255.0;
        GammaLUT[i] = (uint8_t)(pow(intensity, gamma) * 255.0);
    }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Sommerhack LED");

  // this resets all the neopixels to an off state
  strip.setBrightness(255);
  strip.begin();
  strip.show();

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
          Serial.print("Connected to");
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

  fillGammaLUT(2.2);

  Udp.begin(7890);
  for (int i = 0; i < PixelCount; i++)
      strip.setPixelColor(i, 255, 255, 255);
  strip.show();
  Serial.println("Set to all white");
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
    for (int i = offset; i < std::min(size / 3 + offset, PixelCount); i++)
        strip.setPixelColor(i, GammaLUT[*pixrgb++], GammaLUT[*pixrgb++], GammaLUT[*pixrgb++]);
    dirtyshow = true;
}


uint8_t rcv[10 + 300 * 3];

bool autonomous = true;

void clientEventUdp()
{
    int32_t packetSize = 0;
    while ((packetSize = Udp.parsePacket()))
    {
        if (autonomous)
        {
            Serial.println("Switch to non-autonomous mode");
            autonomous = false;
        }
    
        auto len = Udp.read(rcv, sizeof(rcv));
        auto cmdptr = (uint16_t*) rcv;
        auto cmd = cmdptr[0];
        if (cmd == 1111)
            blit_cmd(rcv + sizeof(int16_t), len - sizeof(int16_t)); 
    }
}

int wheel(byte WheelPos)
{
    if (WheelPos < 85) 
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
}

int auto_idx = 0;
int auto_output_ctr = 0;

void runAutonomous()
{
    if (!autonomous)
        return;
    if (!auto_output_ctr)
        Serial.println("Running autonomously");
    ++auto_output_ctr;
    if (auto_output_ctr > 1000)
        auto_output_ctr = 0;
    delay(1);
    
    for (int i = 0; i < PixelCount; ++i)
        strip.setPixelColor(i, wheel((i*10 + auto_idx) & 255));
    dirtyshow = true;
    ++auto_idx;
    if (auto_idx > 255)
        auto_idx = 0;
}


void loop()
{
    clientEventUdp();

    runAutonomous();
    
    if (dirtyshow && strip.canShow())
    {
        strip.show();
        dirtyshow = false;
    }
}
