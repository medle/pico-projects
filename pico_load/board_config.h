
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#define LED_PIN 25

// SSD1306 can operate at 1MHz but it shares bus with 400kHz EEPROM
#define BOARD_I2C_PORT i2c0
#define BOARD_I2C_BAUDRATE (400 * 1000)
#define BOARD_I2C_SDA_PIN 20
#define BOARD_I2C_SCL_PIN 21

// SSD1306 display
#define DISPLAY_I2C_PORT BOARD_I2C_PORT
#define DISPLAY_I2C_ADDRESS 0x3C
#define DISPLAY_I2C_BAUDRATE BOARD_I2C_BAUDRATE 
#define DISPLAY_I2C_SDA_PIN BOARD_I2C_SDA_PIN
#define DISPLAY_I2C_SCL_PIN BOARD_I2C_SCL_PIN

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

// EEPROM chip 24C02 (2Kb) is on bus address 0x50 and has 8-byte pages
#define EEPROM_DEVICEADDR 0x50
#define EEPROM_BYTESPERPAGE 8
#define EEPROM_I2C_BAUDRATE BOARD_I2C_BAUDRATE 
#define EEPROM_I2C_PORT BOARD_I2C_PORT
#define EEPROM_I2C_SDA_PIN BOARD_I2C_SDA_PIN
#define EEPROM_I2C_SCL_PIN BOARD_I2C_SCL_PIN

// SPI bus pins
#define BOARD_SPI_INSTANCE spi0
#define BOARD_SPI_BAUDRATE (1000 * 1000)
#define BOARD_SPI_SCK_PIN 2
#define BOARD_SPI_TX_PIN 3
#define BOARD_SPI_RX_PIN 4
#define BOARD_SPI_CS1_PIN 5 // ADC
#define BOARD_SPI_CS2_PIN 1 // DAC

#define ENCODER_KEY_PIN 22
#define ENCODER_S1_PIN 18
#define ENCODER_S2_PIN 19

#define BUTTON_1_PIN 12
#define BUTTON_2_PIN 13
#define BUTTON_3_PIN 16
#define BUTTON_4_PIN 17

// _BOARD_CONFIG_H_
#endif