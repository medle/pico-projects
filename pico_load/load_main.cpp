#include "load.h"
#include <hardware/gpio.h>
#include <algorithm>
#include "dac_mcp4921.h"
#include "adc_ad7887.h"
#include "eeprom_24cxx.h"
#include "encoder.h"

int g_count;
char g_buffer[100];

#define DAC_MAX_VALUE MCP4921_MAX_VALUE
#define DAC_MAX_VOLTS 1.58 // measured max voltage after divider
#define DAC_VOLT_STEP 0.05

#define ADC_MAX_VOLTS 3.3

int main() 
{
    stdio_init_all();
    sleep_ms(500);

    eeprom_init();
    encoder_init();

    board_led_init(); 
    board_display_init(); 

    // DAC and ADC use the same SPI instance
    mcp4921_init(); 
    ad7887_init();

    const int cycle_ms = 200; 
    while(true) {

        int n = encoder_get_count();
        n = std::max(n, 0);

        int dac_per_step = DAC_MAX_VALUE / (int)(DAC_MAX_VOLTS / DAC_VOLT_STEP); 
        int dac_value = std::min(n * dac_per_step, DAC_MAX_VALUE);
        float dac_volts = dac_value * DAC_MAX_VOLTS / DAC_MAX_VALUE;

        mcp4921_write_dac(dac_value); 

        uint16_t adc = ad7887_read_adc(1); 
        float adc_volts = ad7887_convert_adc_to_volts(adc, ADC_MAX_VOLTS);

        sprintf(g_buffer, "%.2f %.2f", dac_volts, adc_volts);

        // display update lasts 15ms
        board_display_repaint();

        board_led_set(true); 
        sleep_ms(cycle_ms / 2);
        board_led_set(false); 
        sleep_ms(cycle_ms / 2);
    }

    return 0;
}
