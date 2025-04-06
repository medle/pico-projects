
//
// SPI driver for the AD7887 12-bit 2-channel Analog-to-Digital converter.
// SL (06.04.2025)
//
#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include "board_config.h"
#include "adc_ad7887.h"

#define ADC_SPI_INSTANCE BOARD_SPI_INSTANCE
#define ADC_SPI_BAUDRATE BOARD_SPI_BAUDRATE
#define ADC_SPI_SCK_PIN BOARD_SPI_SCK_PIN
#define ADC_SPI_TX_PIN BOARD_SPI_TX_PIN
#define ADC_SPI_RX_PIN BOARD_SPI_RX_PIN
#define ADC_SPI_CS_PIN BOARD_SPI_CS2_PIN

void ad7887_init()
{
    spi_init(ADC_SPI_INSTANCE, ADC_SPI_BAUDRATE);

    // All writes to the AD7887 are 16-bit words. 
    spi_set_format(ADC_SPI_INSTANCE, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(ADC_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(ADC_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(ADC_SPI_RX_PIN, GPIO_FUNC_SPI);

    gpio_init(ADC_SPI_CS_PIN);
    gpio_set_dir(ADC_SPI_CS_PIN, GPIO_OUT);
    gpio_put(ADC_SPI_CS_PIN, 1); 
}

void ad7887_deinit()
{
    spi_deinit(ADC_SPI_INSTANCE);
}

// Enable chip communication.
static inline void cs_select() 
{
    asm volatile("nop \n nop \n nop");
    gpio_put(ADC_SPI_CS_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

// Disable chip communication.
static inline void cs_deselect() 
{
    asm volatile("nop \n nop \n nop");
    gpio_put(ADC_SPI_CS_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

uint16_t ad7887_read_adc(uint8_t channel)
{
    if (channel > 1) return AD7887_MAX_VALUE;

    // Build the 16-bit register word, 8 MSB bits go to control register, 8 LSB bits are ignored.
    uint8_t reg8 = 
      (0 << 7) | // Don’t Care. It doesn’t matter if the bit is 0 or 1.
      (0 << 6) | // A zero must be written to this bit to ensure correct operation of the AD7887.
      (1 << 5) | // Reference Bit. With a 1 in this bit, the on-chip internal reference is disabled.
      (1 << 4) | // A 1 in this bit selects dual-channel mode. In this mode internal reference should be disabled. 
      (channel << 3) | // A 0 in this bit selects the AIN0 input while a 1 in this bit selects the AIN1 input.
      (0 << 2) | // A zero must be written to this bit to ensure correct operation of the AD7887.
      (0 << 1) | // PM1=0. Power management Mode2: PM1=0, PM0=1. In this mode, the AD7887 is always fully powered up. 
      (1 << 0);  // PM0=1.

    uint16_t output16 = ((uint16_t)reg8 << 8); 
    uint16_t input16; 

    cs_select();
    spi_write16_read16_blocking(ADC_SPI_INSTANCE, &output16, &input16, 1);
    cs_deselect();

    return input16;
}
