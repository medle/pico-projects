
#ifndef ADC_AD7887_H
#define ADC_AD7887_H

#include <pico/stdlib.h>

void ad7887_init();
void ad7887_deinit();

/// @brief Reads the 12-bit value from the AD7887 ADC channel.
/// @param data ADC channel number: 0 or 1.
uint16_t ad7887_read_adc(uint8_t channel);

#define AD7887_MAX_VALUE 0x0fff

#endif