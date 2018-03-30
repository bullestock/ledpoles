/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * W2812B I2S Driver
 *
 * Copyright 2015 <>< Charles Lohr, See LICENSE file.
 * WS2812 sender that abuses the I2S interface on the WS2812.
 * https://github.com/cnlohr/esp8266ws2812i2s
 */

#pragma once

struct CRGB;

#include "common.hpp"

#ifdef __linux__
#define ICACHE_FLASH_ATTR
#endif

// You can either have 3 or 4 samples per bit for WS2812s.
//
// 3 samples per bit: Uses less RAM (9 bytes per chip), but takes more
// CPU time, and has more restrictive timing requirement.
//
// 4 samples per bit: Uses more RAM (12 bytes per chip), has higher
// DMA load.

#define WS2812_THREE_SAMPLE
// #define WS2812_FOUR_SAMPLE

// Number of pixels
#define WS_PIXELS 578

// timing for SK6812 LEDs, always uses 4bit samples
//#define SK6812

void ICACHE_FLASH_ATTR ws2812_init();

void ws2812_brightness(int b);

/**
 * Convert WS2812B GRB data to I2S samples. Driver is transmitting
 * continuously and will send the new data from whereever it is at the
 * time.
 *
 * 'buffer' is a pointer to the GRB data which must hold 3 bytes of
 * data for each pixel. If 3-samples-per-bit mode is used, the entire
 * buffer size must be a multiple of 4 bytes and can be zero-padded.
 */
void ws2812_show(const CRGB data[]);

