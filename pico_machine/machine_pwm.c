
#include "machine.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define GPIO_LEFT_HIGH 0
#define GPIO_LEFT_LOW 1
#define GPIO_RIGHT_HIGH 2
#define GPIO_RIGHT_LOW 3

static uint configure_pwm_slice(uint gpio_high, uint gpio_low, uint16_t top, uint16_t match, uint divider);
static uint16_t choose_pwm_top_and_divider(uint hz, bool dual_slope, uint *divider_ptr);

const bool _dual_slope = true;
static bool _is_running = false;
static uint _left_slice;
static uint _right_slice;

void mach_start_pwm(uint hz, float duty)
{
    assert(duty >= 0 && duty <= 100);

    if(_is_running) mach_stop_pwm();

    uint divider;
    uint16_t top = choose_pwm_top_and_divider(hz, _dual_slope, &divider);
    uint16_t match = (uint16_t)(top * duty / 100);  

    _left_slice = configure_pwm_slice(GPIO_LEFT_HIGH, GPIO_LEFT_LOW, top, match, divider);
    _right_slice = configure_pwm_slice(GPIO_RIGHT_HIGH, GPIO_RIGHT_LOW, top, match, divider);

    pwm_set_counter(_left_slice, 0);
    pwm_set_counter(_right_slice, top);

    // start both slices simulthaneously  
    uint32_t mask = (1 << _left_slice) | (1 << _right_slice);
    pwm_set_mask_enabled(mask); 
    _is_running = true;
}

void mach_stop_pwm()
{
     if(_is_running) {
        pwm_set_enabled(_left_slice, false);      
        pwm_set_enabled(_right_slice, false);      
        _is_running = false;
     } 
}

static uint configure_pwm_slice(
    uint gpio_high, uint gpio_low, uint16_t top, uint16_t match, uint divider)
{
    // figure out which slice we just connected to the pin
    uint slice_num = pwm_gpio_to_slice_num(gpio_high);

    // ensure gpios belong to the same slice
    assert(slice_num == pwm_gpio_to_slice_num(gpio_low)); 
    
    // ensure gpios belong to expected slice channels
    assert(pwm_gpio_to_channel(gpio_high) == PWM_CHAN_A);
    assert(pwm_gpio_to_channel(gpio_low) == PWM_CHAN_B);

    gpio_set_function(gpio_high, GPIO_FUNC_PWM);
    gpio_set_function(gpio_low, GPIO_FUNC_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_phase_correct(&config, _dual_slope);
    pwm_config_set_wrap(&config, top);
    pwm_config_set_clkdiv_int(&config, divider);

    // channel A is not inverted, channel B is inverted 
    pwm_config_set_output_polarity(&config, false, true);

    // load into slice but not run
    pwm_init(slice_num, &config, false); 

    // set PWM compare values for both A/B channels 
    pwm_set_both_levels(slice_num, match, match);

    return slice_num; 
}

// For the given frequency and PWM mode returns the TOP/WRAP value, 
// if the divider_ptr is provided stores there the DIVIDER value.
static uint16_t choose_pwm_top_and_divider(
    uint hz, bool dual_slope, uint *divider_ptr)
{
    const uint max_top = 0xFFFF;
    const int max_divider = 256;
    uint32_t clocks_per_second = clock_get_hz(clk_sys);
    uint periods_per_second = hz;

    uint tops_per_second = dual_slope ? 
        (2 * periods_per_second) : periods_per_second;

    uint divider = 1;
    for (; divider <= max_divider; divider++) {
        // minus one cycle by RP2040 datasheet
        uint top = clocks_per_second / (divider * tops_per_second) - 1;
        if (top <= max_top) {
            if (divider_ptr != NULL) *divider_ptr = divider;
            return top;  
        }     
    }

    if (divider_ptr != NULL) *divider_ptr = max_divider; 
    return max_top;
}


