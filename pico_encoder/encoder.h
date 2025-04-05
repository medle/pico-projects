
#ifndef _ENCODER_GLOBALS_H_
#define _ENCODER_GLOBALS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include "board_config.h"

void encoder_display_init();
void encoder_display_repaint();

void encoder_led_init();
void encoder_led_set(bool on);

extern int g_count;
extern char g_buffer[];

// _ENCODER_GLOBALS_H_
#endif