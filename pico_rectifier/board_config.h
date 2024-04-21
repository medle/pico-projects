
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include "hardware/i2c.h"

#define DISPLAY_I2C_PORT i2c0
#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_I2C_BAUDRATE 1000000
#define DISPLAY_I2C_SDA_PIN 20
#define DISPLAY_I2C_SCL_PIN 21

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

#define SOUND_GEN_PIN 14
#define SOUND_GEN_HZ 5000

#define RELAY1_PIN 18
#define RELAY2_PIN 19

#define LED_PIN 25

// _BOARD_CONFIG_H_
#endif