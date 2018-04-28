/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * Main display code
 */

#pragma once

void neomatrix_init();
void neomatrix_run();
void neomatrix_change_program(const char* name);
void neomatrix_set_speed(int fps);
void neomatrix_start_autorun();
void neomatrix_set_brightness(uint8_t brightness);
void neomatrix_set_nightmode(bool nightmode);
