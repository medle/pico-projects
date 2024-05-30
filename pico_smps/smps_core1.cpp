
#include "smps_globals.h"

#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

static uint8_t compute_average(uint8_t *array, uint length);
static void learn_current_sensor_zero_reading();  
static uint8_t get_average_adc_sample();
static float convert_adc_to_amps(uint8_t adc_reading);  

static uint8_t _current_sensor_zero_reading = CURRENT_SENSOR_ZERO_READING;

// Pi Pico ADC sample rate is 600K per second,
// so 100*100=10K and 60 measures per second
static uint8_t _adc_array_level_1[100];
static uint8_t _adc_array_level_2[100];
static int _adc_index_level_1 = 0;
static int _adc_index_level_2 = 0;
static int _overcurrent_count = 0;

void smps_current_sensor_init()
{
    adc_init();

    adc_gpio_init(CURRENT_SENSOR_ADC_PIN);
    adc_select_input(CURRENT_SENSOR_ADC_PIN - FIRST_ADC_PIN);
     
    learn_current_sensor_zero_reading(); 

    memset(_adc_array_level_1, CURRENT_SENSOR_ZERO_READING, sizeof(_adc_array_level_1));
    memset(_adc_array_level_2, CURRENT_SENSOR_ZERO_READING, sizeof(_adc_array_level_2));

    multicore_launch_core1(smps_core1_entry);
}

float smps_current_sensor_get_amps()
{
    return convert_adc_to_amps(get_average_adc_sample());
}

inline uint8_t read_adc_byte()
{
    uint16_t value12bit = adc_read();
    return value12bit >> 4;
}

static void learn_current_sensor_zero_reading()
{
    uint8_t buf[100];
    for (int i = 0; i < sizeof(buf); i++) buf[i] = read_adc_byte();
    _current_sensor_zero_reading = compute_average(buf, sizeof(buf));    
} 

static uint8_t compute_average(uint8_t *array, uint length)
{
    uint sum = 0;
    for(int i = 0; i < length; i++) sum += array[i];
    return (uint8_t)(sum / length);
}

uint8_t get_average_adc_sample()
{
    return compute_average(_adc_array_level_2, sizeof(_adc_array_level_2));
}

float convert_adc_to_amps(uint8_t adc_reading)
{
    bool reversed = false;
    if (adc_reading < _current_sensor_zero_reading) {
        reversed = true;
        adc_reading = _current_sensor_zero_reading +
            (_current_sensor_zero_reading - adc_reading);
    }
      
    if (adc_reading > CURRENT_SENSOR_MAX_READING)
        adc_reading = CURRENT_SENSOR_MAX_READING;

    int delta = adc_reading - _current_sensor_zero_reading;
    int span = CURRENT_SENSOR_MAX_READING - _current_sensor_zero_reading;
    float rate = (float)delta / span;
    float amps = rate * CURRENT_SENSOR_MAX_AMPS;
    return amps * (reversed ? -1 : 1);
}   

// Core1 procedure runs in parallel to the main loop running in Core0.
void smps_core1_entry() 
{
    while (1) {
        _adc_array_level_1[_adc_index_level_1] = read_adc_byte();

        if(++_adc_index_level_1 == sizeof(_adc_array_level_1)) {
            _adc_index_level_1 = 0;
            
            uint8_t avg1 = compute_average(_adc_array_level_1, sizeof(_adc_array_level_1));
            _adc_array_level_2[_adc_index_level_2] = avg1;
            if (++_adc_index_level_2 == sizeof(_adc_array_level_2)) { 
              _adc_index_level_2 = 0;

                // at the end of each buffer cycle check amp limit
                float amps = convert_adc_to_amps(get_average_adc_sample());
                if (abs(amps) > _smps_config.amp_limit) {
                    smps_enter_alarm_mode();
                    _overcurrent_count += 1;
                }
            }
        }
    }
}
