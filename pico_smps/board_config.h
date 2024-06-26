
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include "hardware/i2c.h"

#define DISPLAY_I2C_PORT i2c0
#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_I2C_BAUDRATE 1000000
#define DISPLAY_I2C_SDA_PIN 20
#define DISPLAY_I2C_SCL_PIN 21

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

#define EEPROM_I2C_PORT i2c1
#define EEPROM_I2C_SDA_PIN 10
#define EEPROM_I2C_SCL_PIN 11

// EEPROM chip 24C02 (2Kb) is on bus address 0x50 and has 8-byte pages
#define EEPROM_DEVICEADDR 0x50
#define EEPROM_BYTESPERPAGE 8
#define EEPROM_I2C_BAUDRATE (100 * 1000)

#define BUTTON_1_PIN 12
#define BUTTON_2_PIN 13
#define BUTTON_3_PIN 16
#define BUTTON_4_PIN 17

#define BUTTON_ON_PIN BUTTON_1_PIN
#define BUTTON_DOWN_PIN BUTTON_2_PIN
#define BUTTON_MODE_PIN BUTTON_3_PIN
#define BUTTON_UP_PIN BUTTON_4_PIN

#define SOUND_GEN_PIN 14
#define SOUND_GEN_HZ 5000

#define RELAY1_PIN 18
#define RELAY2_PIN 19
#define PWM_PIN 0
#define LED_PIN 25

#define POWER_INPUT_PIN 9

// These three are used in PIO and must go in one sequence.
#define LIMITER_FIRST_PIN 2
#define LIMITER_PWM_INPUT_PIN LIMITER_FIRST_PIN
#define LIMITER_INPUT_PIN (LIMITER_FIRST_PIN + 1)
#define LIMITER_OUTPUT_PIN (LIMITER_FIRST_PIN + 2)

#define FIRST_ADC_PIN 26
#define CURRENT_SENSOR_ADC_PIN 26
#define CURRENT_SENSOR_MAX_AMPS 40
#define CURRENT_SENSOR_ZERO_READING 129
#define CURRENT_SENSOR_MAX_READING (int)(128 + 128 * ((4.5 - 2.5) / 2.5))

// _BOARD_CONFIG_H_
#endif