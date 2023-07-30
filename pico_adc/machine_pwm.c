
#include "machine.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static uint configure_pwm_slice(uint gpio_high, uint gpio_low, uint16_t top, uint16_t match, uint divider);
static uint16_t choose_pwm_top_and_divider(uint hz, bool dual_slope, uint *divider_ptr);
static void on_wrap_callback();

const uint GPIO_LEFT_HIGH = 0; 
const uint GPIO_LEFT_LOW = 1;
const uint GPIO_RIGHT_HIGH = 2;
const uint GPIO_RIGHT_LOW = 3;

const bool USE_DUAL_SLOPE = true;
static bool _slices_are_running = false;

static uint _left_slice;
static uint _right_slice;

static void (*_user_wrap_handler)();

void mach_pwm_start(uint hz, float duty, void (*wrap_handler)())
{
    assert(duty >= 0 && duty <= 100);
    if(_slices_are_running) mach_pwm_stop();

    uint divider;
    uint16_t top = choose_pwm_top_and_divider(hz, USE_DUAL_SLOPE, &divider);
    uint16_t match = (uint16_t)(top * duty / 100);  

    _left_slice = configure_pwm_slice(GPIO_LEFT_HIGH, GPIO_LEFT_LOW, top, match, divider);
    _right_slice = configure_pwm_slice(GPIO_RIGHT_HIGH, GPIO_RIGHT_LOW, top, match, divider);

    if (wrap_handler != NULL) {
        _user_wrap_handler = wrap_handler; 

        // Mask our slice's IRQ output into the PWM block's single interrupt line,
        // and register our interrupt handler
        pwm_clear_irq(_left_slice); 
        pwm_set_irq_enabled(_left_slice, true);
        irq_set_exclusive_handler(PWM_IRQ_WRAP, on_wrap_callback);
        irq_set_enabled(PWM_IRQ_WRAP, true);
    }

    pwm_set_counter(_left_slice, 0);
    pwm_set_counter(_right_slice, top);

    // start both slices simulthaneously  
    uint32_t mask = (1 << _left_slice) | (1 << _right_slice);
    pwm_set_mask_enabled(mask); 
    _slices_are_running = true;
}

void mach_pwm_stop()
{
     if(_slices_are_running) {
        pwm_set_enabled(_left_slice, false);      
        pwm_set_enabled(_right_slice, false);      
        _slices_are_running = false;

        if(_user_wrap_handler != NULL) {
            pwm_set_irq_enabled(_left_slice, false);
            irq_set_enabled(PWM_IRQ_WRAP, false);
            _user_wrap_handler = NULL;
        }
     } 
}

static void on_wrap_callback()
{
    // determine which slice has caused the irq:
    uint32_t mask = pwm_get_irq_status_mask();
    if(mask & (1 << _left_slice)) {
        pwm_clear_irq(_left_slice);
        if(_user_wrap_handler != NULL) _user_wrap_handler();
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
    pwm_config_set_phase_correct(&config, USE_DUAL_SLOPE);
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


