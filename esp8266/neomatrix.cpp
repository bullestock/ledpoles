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

void neomatrix_init()
{
    currentFactory = ProgramFactory::first;
    current = currentFactory->launch();
    startTime = millis();
}

#define RUNTIME 16000
#define FADETIME 1000
#define FADEIN FADETIME
#define FADEOUT (RUNTIME-FADETIME)
#define MAX_BRIGHT 255

void program_loop()
{
    uint32_t now = millis();
    uint32_t prgTime = now - startTime;
    if (prgTime < FADEIN)
    {
        Serial.println("Fade in");
        FastLED.setBrightness((prgTime * MAX_BRIGHT) / FADEIN);
    }
    else if (prgTime < FADEOUT)
    {
        FastLED.setBrightness(MAX_BRIGHT);
        Serial.println("Max brightness");
    }
    else if (prgTime < RUNTIME)
    {
        Serial.println("Fade out");
        FastLED.setBrightness(((RUNTIME - prgTime) * MAX_BRIGHT) / FADETIME);
    }
    else
    {
        FastLED.setBrightness(0);
        delete current;
        currentFactory = currentFactory->next;
        if (!currentFactory)
            currentFactory = ProgramFactory::first;
        current = currentFactory->launch();
        startTime = now;
    }
}

void neomatrix_run()
{
    current->run();
    show();
  //!!_show(frame);
}
