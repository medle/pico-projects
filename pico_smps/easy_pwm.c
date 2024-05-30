
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "easy_pwm.h"

static uint16_t choose_pwm_top_and_divider(
    uint periods_per_second, bool dual_slope, uint *divider_ptr)
{
    const uint max_top = 0xFFFF;
    const int max_divider = 256;
    uint32_t clocks_per_second = clock_get_hz(clk_sys);

    uint tops_per_second = dual_slope ? 
        (2 * periods_per_second) : periods_per_second;

    uint divider = 1;
    for (; divider <= max_divider; divider++) {
        // minus one cycle by RP2040 datasheet
        uint top = clocks_per_second / (divider * tops_per_second) - 1;
        if (top <= max_top) {
            *divider_ptr = divider;
            return top;  
        }     
    }

    *divider_ptr = max_divider; 
    return max_top;
}

void easy_pwm_enable(uint gpio, uint hz, float duty)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    gpio_set_function(gpio + 1, GPIO_FUNC_PWM);

    // figure out which slice we just connected to the pin
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // dual-slope operation
    const bool dual_slope = false;
    pwm_config config = pwm_get_default_config();
    uint divider;
    uint16_t count_top = choose_pwm_top_and_divider(hz, dual_slope, &divider);
    pwm_config_set_phase_correct(&config, dual_slope);
    pwm_config_set_wrap(&config, count_top);
    pwm_config_set_clkdiv_int(&config, divider);

    // channel A is not inverted, channel B is inverted 
    pwm_config_set_output_polarity(&config, false, true);

    // load into slice but not run
    pwm_init(slice_num, &config, false); 

    // set PWM compare values for both A/B channels 
    uint high_cycles = (uint)(count_top * duty);
    pwm_set_both_levels(slice_num, high_cycles, high_cycles);

    pwm_set_counter(slice_num, 0);

    // start the counter
    pwm_set_enabled(slice_num, true);
}

void easy_pwm_disable(uint gpio)
{
    // figure out which slice we just connected to the pin
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice_num, false);
    gpio_set_function(gpio, GPIO_FUNC_NULL);
}
