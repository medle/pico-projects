
#include "machine.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static uint configure_pwm_slice(
    uint gpio_high, uint gpio_low, uint16_t top, uint16_t match, uint divider);
static uint16_t choose_pwm_top_and_divider(uint hz, bool dual_slope, uint *divider_ptr);
static void set_slice_match_levels(uint slice_num, uint16_t match);
static void on_pwn_wrap();
static void bring_gpio_low(int gpio);
static void bring_all_outputs_low();

static uint _dead_clocks = 0;
static bool _one_sided = false;

const uint _gpio_left_high = 0; 
const uint _gpio_left_low = 1;
const uint _gpio_right_high = 2;
const uint _gpio_right_low = 3;

const bool _dual_slope = true;
static bool _is_running = false;

static uint _left_slice;
static uint _right_slice;

static uint _last_divider = 0;
static uint16_t _last_top = 0;
static uint _last_level = 0;

static void (*_user_wrap_handler)();

/// @brief Initialize the PWM module, bring all outputs low.
void mach_pwm_init()
{
    bring_all_outputs_low();
}

/// @brief Set the number of MCU dead cycles between HIGH side going low and LOW side going high.
void mach_pwm_set_dead_clocks(uint dead_clocks)
{
    _dead_clocks = dead_clocks;
}

/// @brief Turn on or off the one-sided operation.
/// @param one_sided 
void mach_pwm_set_one_sided(bool one_sided)
{   
    _one_sided = one_sided;
}

/// @brief Returns the last used PWM configuration values.
pwm_config_t mach_pwm_get_config()
{
    pwm_config_t config;
    config.divider = _last_divider;
    config.wrap = _last_top;
    config.level = _last_level;
    return config;
}

static uint16_t compute_dual_slope_match(uint16_t top, float duty)
{
    // Dual-slope operation produces high signal when counter is below
    // the match value and low signal when counter is above the match
    // value. Thus to produce the pulse with given duty cycle one needs
    // to use the (100% - duty) value. 
    return (uint16_t)(top * (100 - duty) / 100);  
}

/// @brief Starts the PWM signals on outputs.
/// @param hz Frequency of PWM in cycles per second.
/// @param duty Duty cycle in 100%.
/// @param wrap_handler Callback handler to be called at the end of each cycle.
void mach_pwm_start(uint hz, float duty, void (*wrap_handler)())
{
    assert(duty >= 0 && duty <= 100);
    if(_is_running) mach_pwm_stop();

    uint divider;
    uint16_t top = choose_pwm_top_and_divider(hz, _dual_slope, &divider);
    uint16_t match = compute_dual_slope_match(top, duty);  

    _last_divider = divider;
    _last_top = top; 
    _last_level = match;

    _left_slice = configure_pwm_slice(_gpio_left_high, _gpio_left_low, top, match, divider);
    _right_slice = configure_pwm_slice(_gpio_right_high, _gpio_right_low, top, match, divider);

    if (wrap_handler != NULL) {
        _user_wrap_handler = wrap_handler; 

        // Mask our slice's IRQ output into the PWM block's single interrupt line,
        // and register our interrupt handler
        pwm_clear_irq(_left_slice); 
        pwm_set_irq_enabled(_left_slice, true);
        irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwn_wrap);
        irq_set_enabled(PWM_IRQ_WRAP, true);
    }

    pwm_set_counter(_left_slice, 0);
    pwm_set_counter(_right_slice, top);

    // start both slices simulthaneously  
    uint32_t mask = (1 << _left_slice) | (1 << _right_slice);
    pwm_set_mask_enabled(mask); 
    _is_running = true;
}

/// @brief With running PWM slices change the frequency and the duty cycle.
/// @param hz New frequency
/// @param duty New duty cycle
void mach_pwm_change_waveform(uint hz, float duty)
{
    if(!_is_running) return;

    uint new_divider;
    uint16_t new_top = choose_pwm_top_and_divider(hz, _dual_slope, &new_divider);
    uint16_t new_match = compute_dual_slope_match(new_top, duty);  

    if(new_divider != _last_divider) {
        pwm_set_clkdiv_int_frac(_left_slice, new_divider, 0); 
        pwm_set_clkdiv_int_frac(_right_slice, new_divider, 0); 
        _last_divider = new_divider;
    }

    pwm_set_wrap(_left_slice, new_top);
    pwm_set_wrap(_right_slice, new_top);

    set_slice_match_levels(_left_slice, new_match);            
    set_slice_match_levels(_right_slice, new_match);            
    _last_level = new_match;

    // right counter should always be TOP clocks further than the left
    // counter, when TOP changes add/subtract the right counter clocks
    if (_last_top != new_top) {      
        if (_last_top > new_top) { // TOP becomes smaller
            uint minus = _last_top - new_top;
            do { 
                pwm_retard_count(_right_slice); 
            } while(--minus > 0);
        } else { // TOP becomes larger
            uint plus = new_top - _last_top;
            do { 
                pwm_advance_count(_right_slice); 
            } while(--plus > 0);
        }
        _last_top = new_top; 
    }
}

/// @brief Returns true if PWM is running now.
/// @return True if running, false if not running.
bool mach_pwm_is_running()
{
    return _is_running;
}

/// @brief Stops the PWM signals if running now.
void mach_pwm_stop()
{
     if(_is_running) {
        pwm_set_enabled(_left_slice, false);      
        pwm_set_enabled(_right_slice, false);      
        _is_running = false;

        if(_user_wrap_handler != NULL) {
            pwm_set_irq_enabled(_left_slice, false);
            irq_set_enabled(PWM_IRQ_WRAP, false);
            _user_wrap_handler = NULL;
        }

        bring_all_outputs_low(); 
     } 
}

static void on_pwn_wrap()
{
    // determine which slice has caused the irq:
    uint32_t mask = pwm_get_irq_status_mask();
    if(mask & (1 << _left_slice)) {
        pwm_clear_irq(_left_slice);
        if(_user_wrap_handler != NULL) _user_wrap_handler();
    }
}

static void set_slice_match_levels(uint slice_num, uint16_t match)
{
    // adjust the HIGH output for the dead cycles 
    uint16_t match_a = (_dead_clocks < match) ? (match - _dead_clocks) : 0;

    // set PWM compare values for both A/B channels 
    pwm_set_both_levels(slice_num, match_a, match);
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
    set_slice_match_levels(slice_num, match);

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

    // functions pwm_advance_count() / pwm_retard_count() will hang
    // with divider = 1 so never use it and begin with divider = 2
    uint divider = 2;
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

static void bring_gpio_low(int gpio)
{
    gpio_init(gpio); // set function GPIO_FUNC_SIO
    gpio_set_pulls(gpio, false, true); // pull-down
    gpio_set_dir(gpio, GPIO_OUT); // output direction
    gpio_put(gpio, false); // low state
}

static void bring_all_outputs_low()
{
    bring_gpio_low(_gpio_left_high);
    bring_gpio_low(_gpio_left_low);
    bring_gpio_low(_gpio_right_high);
    bring_gpio_low(_gpio_right_low);
}


