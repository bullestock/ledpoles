/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Main display code
 */

#include <FastLED.h>

#include "common.hpp"
#include "program.hpp"

// static pixel* frame;
// pixel* pixels;

Program* current = nullptr;
uint32_t startTime = 0;
ProgramFactory* currentFactory = nullptr;

extern void show();
extern void clear_all();

void neomatrix_init()
{
    currentFactory = ProgramFactory::first;
    current = currentFactory->launch();
    Serial.printf("Launched %s\n", currentFactory->name);
    startTime = millis();
}

#define RUNTIME 16000
#define FADETIME 3000
#define FADEIN FADETIME
#define FADEOUT (RUNTIME-FADETIME)
#define MAX_BRIGHT 255

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
            currentFactory = ProgramFactory::first;
        current = currentFactory->launch();
        Serial.printf("Launched %s\n", currentFactory->name);
        startTime = now;
    }
}

void neomatrix_run()
{
    program_loop();
    if (current->run())
        show();
}
