
#ifndef _SMPS_GLOBALS_H_
#define _SMPS_GLOBALS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "board_config.h"

void smps_enter_alarm_mode();

void smps_display_init();
void smps_display_repaint();

#define SMPS_MODE_HZ 0
#define SMPS_MODE_DUTY 1
#define SMPS_MODE_LIMIT 2

// Global state.
extern bool _smps_pwm_running;
extern int _smps_mode;
extern int _smps_cycle_count;
extern volatile bool _smps_limit_occured;

// PIO functions.
void smps_pio_start_limited_repeater();

// Core1 current sensor functions.
void smps_core1_entry(); 
void smps_current_sensor_init();
float smps_current_sensor_get_amps();

// Utility functions.
void smps_led_init();
void smps_led_set(bool on);
void smps_sound_enable(bool enable);
void smps_sound_play_beeps(int n);

// Configuration written to EEPROM.
typedef struct config_t {
    uint magic;
    uint pwm_hz;
    float pwm_duty;
    volatile float amp_limit;
} config_t;

extern config_t _smps_config;

void smps_config_init();
void smps_config_restore();
void smps_config_save();

// _SMPS_GLOBALS_H_
#endif