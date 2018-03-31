/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Main display code
 */

#include "common.hpp"
#include "defs.h"
#include "display.hpp"
#include "neomatrix.hpp"
#include "program.hpp"
#include "stripmode.hpp"
#include "ws2812_i2s.hpp"
#include <FastLED.h>

Program* current = nullptr;
uint32_t startTime = 0;
ProgramFactory* currentFactory = nullptr;
static CRGB* swap_buf = nullptr;

extern void show();

void neomatrix_init()
{
    ws2812_init();
}

void neomatrix_start()
{
    currentFactory = ProgramFactory::first;
    current = currentFactory->launch();
    Serial.printf("Launched %s\n", currentFactory->name);
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
        ws2812_brightness((prgTime * max_brightness) / FADEIN);
    }
    else if (prgTime < FADEOUT)
    {
        auto brightness = max_brightness;
        if (night_mode)
            brightness = min(brightness, 64);
        ws2812_brightness(brightness);
    }
    else if (prgTime < RUNTIME)
    {
        ws2812_brightness(((RUNTIME - prgTime) * max_brightness) / FADETIME);
    }
    else
    {
        ws2812_brightness(0);
        clear_all();
        show();
        delete current;

        do
        {
            currentFactory = currentFactory->next;
            if (!currentFactory)
            {
                currentFactory = ProgramFactory::first;
            }
            current = currentFactory->launch();
        }
        while (night_mode && !current->allow_night_mode());
        
        Serial.printf("Launched %s\n", currentFactory->name);
        startTime = now;
    }
}

void neomatrix_run(CRGB* pixels)
{
    if (auto_program_switch)
        program_loop();
    if (run_autonomously && current->run())
        neomatrix_show(pixels);
}

void neomatrix_show(CRGB* pixels)
{
    // RGB -> GRB
    if (!swap_buf)
        swap_buf = new CRGB[NUM_LEDS];
    
    for (int i = 0; i < NUM_LEDS; ++i)
    {
        swap_buf[i].r = pixels[i].g;
        swap_buf[i].g = pixels[i].r;
        swap_buf[i].b = pixels[i].b;
    }
    ws2812_show(swap_buf);
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
    ws2812_brightness(max_brightness);
}

void neomatrix_set_speed(int fps)
{
    if (current)
        current->limiter.setFps(fps);
}

void neomatrix_set_brightness(uint8_t brightness)
{
    max_brightness = brightness;
    ws2812_brightness(max_brightness);
}

void neomatrix_set_nightmode(bool nightmode)
{
    night_mode = nightmode;
}

void neomatrix_start_autorun()
{
    run_autonomously = true;
    auto_program_switch = true;
}
