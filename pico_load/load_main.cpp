
#include <hardware/gpio.h>
#include "pico/multicore.h"
#include <algorithm>

#include "load.h"
#include "dac_mcp4921.h"
#include "adc_ad7887.h"
#include "eeprom_24cxx.h"
#include "encoder.h"

int g_count;
char g_buffer[100];

static void core1_entry(); 

#define DAC_MAX_VALUE MCP4921_MAX_VALUE
#define DAC_MAX_VOLTS 1.58 // measured max voltage after divider
#define DAC_VOLT_STEP 0.02
#define ADC_MAX_VOLTS 3.3

static uint16_t _adc1_value;
static uint16_t _adc2_value;

static int _encoder_value;
#define ENCODER_MIN_VALUE 0
#define ENCODER_MAX_VALUE ((int)(DAC_MAX_VOLTS / DAC_VOLT_STEP))

static int _measures_per_second = 0;

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

    multicore_launch_core1(core1_entry);

    const int cycle_ms = 200; 
    while(true) {

        //int dac_per_step = DAC_MAX_VALUE / (int)(DAC_MAX_VOLTS / DAC_VOLT_STEP); 
        //int dac_value = std::min(n * dac_per_step, DAC_MAX_VALUE);
        //float dac_volts = dac_value * DAC_MAX_VOLTS / DAC_MAX_VALUE;
        //mcp4921_write_dac(dac_value); 

        float adc1_volts = ad7887_convert_adc_to_volts(_adc1_value, ADC_MAX_VOLTS);
        float adc2_volts = ad7887_convert_adc_to_volts(_adc2_value, ADC_MAX_VOLTS);

        sprintf(g_buffer, "%.2f %.2f", adc1_volts, adc2_volts);
        g_count = _measures_per_second;

        // display update lasts 15ms
        board_display_repaint();

        board_led_set(true); 
        sleep_ms(cycle_ms / 2);
        board_led_set(false); 
        sleep_ms(cycle_ms / 2);
    }

    return 0;
}

static void process_encoder_changes()
{
    static int _last_raw = 0;

    int raw = encoder_get_count();
    if (raw == _last_raw) return;

    int delta = raw - _last_raw;
    _last_raw = raw;

    int adjusted = _encoder_value + delta;
    _encoder_value = std::max(ENCODER_MIN_VALUE, std::min(ENCODER_MAX_VALUE, adjusted));
}

static void core1_entry() 
{
    int measure_count = 0;
    uint32_t start_time = time_us_32();

    // this loop cycles at the speed of 15kHz
    while (true) { 

        process_encoder_changes();  

        mcp4921_write_dac(DAC_MAX_VALUE / 2); 

        _adc1_value = ad7887_read_adc(0); 
        _adc2_value = ad7887_read_adc(1); 

        measure_count += 1;

        uint32_t stop_time = time_us_32();
        int elapsed = (int)((stop_time - start_time) * 1e-6f);
        if (elapsed > 0) {
            _measures_per_second = measure_count;
            measure_count = 0;
            start_time = stop_time;
        }
    }
}
