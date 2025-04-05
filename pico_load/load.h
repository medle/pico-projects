
#ifndef _LOAD_GLOBALS_H_
#define _LOAD_GLOBALS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include "board_config.h"

void board_display_init();
void board_display_repaint();

void board_led_init();
void board_led_set(bool on);

extern int g_count;
extern char g_buffer[];

// _LOAD_GLOBALS_H_
#endif