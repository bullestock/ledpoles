/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Main display code
 */

#include <FastLED.h>

#include "common.hpp"
#include "display.hpp"
#include "program.hpp"
#include "stripmode.hpp"

Program* current = nullptr;
uint32_t startTime = 0;
ProgramFactory* currentFactory = nullptr;

extern void show();

void neomatrix_init()
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
#define MAX_BRIGHT 255

bool run_autonomously = true;
bool auto_program_switch = true;

void program_loop()
{
    uint32_t now = millis();
    uint32_t prgTime = now - startTime;
    if (prgTime < FADEIN)
    {
        FastLED.setBrightness((prgTime * MAX_BRIGHT) / FADEIN);
    }
    else if (prgTime < FADEOUT)
    {
        FastLED.setBrightness(MAX_BRIGHT);
    }
    else if (prgTime < RUNTIME)
    {
        FastLED.setBrightness(((RUNTIME - prgTime) * MAX_BRIGHT) / FADETIME);
    }
    else
    {
        FastLED.setBrightness(0);
        clear_all();
        show();
        delete current;
        currentFactory = currentFactory->next;
        if (!currentFactory)
        {
            currentFactory = ProgramFactory::first;
            auto new_strip_mode = static_cast<StripMode>(static_cast<int>(get_strip_mode())+1);
            if (new_strip_mode >= StripMode::Last)
                new_strip_mode = StripMode::First;
            set_strip_mode(new_strip_mode);
        }
        current = currentFactory->launch();
        Serial.printf("Launched %s\n", currentFactory->name);
        startTime = now;
    }
}

void neomatrix_run()
{
    if (auto_program_switch)
        program_loop();
    if (run_autonomously && current->run())
        show();
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
}

void neomatrix_set_speed(int fps)
{
    if (current)
        current->limiter.setFps(fps);
}

void neomatrix_start_autorun()
{
    run_autonomously = true;
    auto_program_switch = true;
}
