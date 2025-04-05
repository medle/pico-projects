
#include "load.h"
#include <hardware/gpio.h>
#include <algorithm>
#include "dac_mcp4921.h"
#include "eeprom_24cxx.h"
#include "encoder.h"

int g_count;
char g_buffer[100];

#define DAC_MAX_VALUE MCP4921_MAX_VALUE
#define MAX_VOLTS 3.3
#define VOLT_STEP 0.05

// see button debounce library https://github.com/TuriSc/RP2040-Button
void gpio_callback(uint gpio, uint32_t events);

int falls = 0;
int rises = 0;

int main() 
{
    stdio_init_all();
    sleep_ms(500);

    eeprom_init();
    mcp4921_init();
    encoder_init();

    board_led_init(); 
    board_display_init(); 

    //uint32_t hey;
    //int ret = eeprom_read_bytes(0, (uint8_t *)&hey, sizeof(hey));
    //printf("eeprom read=%d 0x%x\n", ret, hey);

    sprintf(g_buffer, "ready");
    board_display_repaint();

    const int cycle_ms = 100; 

    while(true) {

        int n = encoder_get_count();
        if (n < 0) n = 0;

        int dac_per_step = DAC_MAX_VALUE / (int)(MAX_VOLTS / VOLT_STEP); 
        int dac_value = std::min(n * dac_per_step, DAC_MAX_VALUE);
        float volts = dac_value * MAX_VOLTS / DAC_MAX_VALUE;
        sprintf(g_buffer, "v=%.2f %d", volts);
        mcp4921_write_dac(dac_value); 
        
        //sprintf(g_buffer, "e%d r%d f%d", n, rises, falls); 

        // display update lasts 15ms
        board_display_repaint();

        board_led_set(true); 
        sleep_ms(cycle_ms);
        board_led_set(false); 
        sleep_ms(cycle_ms);
    }

    return 0;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == ENCODER_KEY_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) rises += 1; 
        if (events & GPIO_IRQ_EDGE_FALL) falls += 1; 
    }

    //sprintf(g_buffer, "%d r%d f%d", last, rises, falls);
}

