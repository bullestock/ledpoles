/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Main display code
 */

#include <FastLED.h>
#include "SSD1306.h" 
#include <esp_timer.h>
#include <WiFi.h>

#include "common.hpp"
#include "display.hpp"
#include "program.hpp"
#include "stripmode.hpp"

Program* current = nullptr;
uint32_t startTime = 0;
ProgramFactory* currentFactory = nullptr;

extern SSD1306 display;

extern void show();

void neomatrix_init()
{
    currentFactory = ProgramFactory::first;
    current = currentFactory->launch();
    char buf[80];
    sprintf(buf, "Launched %s", currentFactory->name);
    Serial.printf("%s\n", buf);
#if 0
    display.clear();
    display.drawString(0, 0, buf);
    display.display();
#endif
    startTime = millis();
}

#define RUNTIME 60000
#define FADETIME 3000
#define FADEIN FADETIME
#define FADEOUT (RUNTIME-FADETIME)

static int max_brightness = 255;
static bool night_mode = false;

bool run_autonomously = true;
bool auto_program_switch = true;

void program_loop()
{
    uint32_t now = millis();
    uint32_t prgTime = now - startTime;
    if (prgTime < FADEIN)
    {
        FastLED.setBrightness((prgTime * max_brightness) / FADEIN);
    }
    else if (prgTime < FADEOUT)
    {
        auto brightness = max_brightness;
        if (night_mode)
            brightness = std::min(brightness, 64);
        FastLED.setBrightness(brightness);
    }
    else if (prgTime < RUNTIME)
    {
        FastLED.setBrightness(((RUNTIME - prgTime) * max_brightness) / FADETIME);
    }
    else
    {
        FastLED.setBrightness(0);
        clear_all();
        show();
        delete current;

        do
        {
            currentFactory = currentFactory->next;
            if (!currentFactory)
            {
                currentFactory = ProgramFactory::first;
                auto new_strip_mode = static_cast<StripMode>(static_cast<int>(get_strip_mode())+1);
                if (new_strip_mode >= StripMode::Last)
                    new_strip_mode = StripMode::First;
                set_strip_mode(new_strip_mode);
                Serial.printf("New strip mode %d\n", new_strip_mode);
            }
            current = currentFactory->launch();
        }
        while (night_mode && !current->allow_night_mode());
        
        char buf[80];
        sprintf(buf, "Auto: %s", currentFactory->name);
        Serial.printf("%s\n", buf);
        display.clear();
        display.drawString(0, 0, buf);
        const int secs = esp_timer_get_time()/1000000;
        int mins = secs/60;
        const int hours = mins/60;
        mins -= hours*60;
        sprintf(buf, "%d up %02d:%02d", (int) WiFi.RSSI(), hours, mins);
        Serial.printf("%s\n", buf);
        display.drawString(0, 16, buf);
        display.display();
        startTime = now;
    }
}

void neomatrix_run()
{
    if (auto_program_switch)
        program_loop();
    if (run_autonomously)
    {
        if (current->run())
            show();
    }
    else
    {
        display.clear();
        display.drawString(0, 0, "External control");
        display.display();
    }
}

void neomatrix_change_program(const char* name)
{
    auto p = ProgramFactory::get(name);
    if (!p)
    {
        Serial.println("Not found");
        return;
    }
    current = p->launch();
    auto_program_switch = false;
    clear_all();
    FastLED.setBrightness(max_brightness);
    display.clear();
    char buf[80];
    sprintf(buf, "Manual: %s", currentFactory->name);
    display.drawString(0, 0, buf);
    display.display();
}

void neomatrix_set_speed(int fps)
{
    if (current)
        current->limiter.setFps(fps);
}

void neomatrix_set_brightness(uint8_t brightness)
{
    max_brightness = brightness;
    FastLED.setBrightness(max_brightness);
}

void neomatrix_set_nightmode(bool nightmode)
{
    night_mode = nightmode;
}

void neomatrix_start_autorun()
{
    run_autonomously = true;
    auto_program_switch = true;
    display.clear();
    display.drawString(0, 0, "Autonomous mode");
    display.display();
}
