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
