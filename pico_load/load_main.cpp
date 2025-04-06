
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
#define MAX_VOLTS 1.58 // measured max voltage after divider
#define VOLT_STEP 0.05

int main() 
{
    stdio_init_all();
    sleep_ms(500);

    eeprom_init();
    encoder_init();

    board_led_init(); 
    board_display_init(); 

    //uint32_t hey;
    //int ret = eeprom_read_bytes(0, (uint8_t *)&hey, sizeof(hey));
    //printf("eeprom read=%d 0x%x\n", ret, hey);

    const int cycle_ms = 200; 
    while(true) {

        int n = encoder_get_count();
        n = std::max(n, 0);

        int dac_per_step = DAC_MAX_VALUE / (int)(MAX_VOLTS / VOLT_STEP); 
        int dac_value = std::min(n * dac_per_step, DAC_MAX_VALUE);
        float volts = dac_value * MAX_VOLTS / DAC_MAX_VALUE;

        mcp4921_init();
        mcp4921_write_dac(dac_value); 
        mcp4921_deinit();

        ad7887_init();
        uint16_t adc = ad7887_read_adc(1); 
        ad7887_deinit();

        sprintf(g_buffer, "v=%.2f %d", volts, adc);

        // display update lasts 15ms
        board_display_repaint();

        board_led_set(true); 
        sleep_ms(cycle_ms / 2);
        board_led_set(false); 
        sleep_ms(cycle_ms / 2);
    }

    return 0;
}
