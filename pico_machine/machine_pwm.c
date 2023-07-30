
#include "machine.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

static void bring_all_outputs_low();

static uint _dead_clocks = 0;
static bool _one_sided = false;

const uint GPIO_LEFT_HIGH = 0;  // GP0  
const uint GPIO_LEFT_LOW = 1;   // GP1
const uint GPIO_RIGHT_HIGH = 2; // GP2
const uint GPIO_RIGHT_LOW = 3;  // GP3
const bool USE_DUAL_SLOPE = true;

static bool _slices_are_running = false;
static uint _left_slice;
static uint _right_slice;

static volatile bool _reloading_wrap_level = false;
static uint _selected_divider = 0;
static uint16_t _selected_wrap = 0;
static uint _selected_level = 0;

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
void mach_pwm_set_one_sided(bool one_sided)
{   
    _one_sided = one_sided;
}

/// @brief Returns the last used PWM configuration values.
pwm_config_t mach_pwm_get_config()
{
    pwm_config_t config;
    config.divider = _selected_divider;
    config.wrap = _selected_wrap;
    config.level = _selected_level;
    return config;
}

static inline uint32_t make_both_slices_bitmask()
{
    return (1 << _left_slice) | (1 << _right_slice);
}

static void set_slice_match_levels(uint slice_num, uint16_t top, uint16_t match)
{
    // adjust the HIGH output channel A for the dead cycles,
    // left slice is "not shifted" and it's HIGH channel A match gets the minus DT cycles,
    // right slice is "shifted" and it's HIGH channel A match gets the plus DT cycles
    uint16_t match_a;
    if(slice_num == _right_slice) {
        uint16_t inverted_match = top - match;  
        uint16_t adjusted_match = (inverted_match + _dead_clocks);
        match_a = (adjusted_match <= top) ? adjusted_match : top;
        match = inverted_match;
    } else {
        match_a = (_dead_clocks < match) ? (match - _dead_clocks) : 0;
    }

    // set PWM compare values for both A/B channels 
    pwm_set_both_levels(slice_num, match_a, match);
}

static uint configure_pwm_slice(
    uint gpio_high, uint gpio_low, uint16_t top, uint16_t match, uint divider, bool phase_shifted)
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

    // channel A and channel B are inverted again each other
    bool inverted = phase_shifted;
    pwm_config_set_output_polarity(&config, inverted, !inverted);

    // load into slice but not run
    pwm_init(slice_num, &config, false); 

    // set PWM compare values for both A/B channels 
    set_slice_match_levels(slice_num, top, match);

    return slice_num; 
}

static void on_wrap_callback()
{
    // determine which slice has caused the irq:
    uint32_t mask = pwm_get_irq_status_mask();
    if(mask & (1 << _left_slice)) {
        pwm_clear_irq(_left_slice);

        if(_reloading_wrap_level) {
            pwm_set_wrap(_left_slice, _selected_wrap);
            pwm_set_wrap(_right_slice, _selected_wrap);
            set_slice_match_levels(_left_slice, _selected_wrap, _selected_level);            
            set_slice_match_levels(_right_slice, _selected_wrap, _selected_level);            
            _reloading_wrap_level = false;
        }

        if(_user_wrap_handler != NULL) _user_wrap_handler();
    }
}

/// @brief Starts the PWM signals on outputs.
/// @param hz Frequency of PWM in cycles per second.
/// @param duty Duty cycle in 100%.
/// @param wrap_handler Callback handler to be called at the end of each cycle.
void mach_pwm_start(uint hz, float duty, void (*wrap_handler)())
{
    assert(duty >= 0 && duty <= 100);
    if(_slices_are_running) mach_pwm_stop();

    uint divider;
    uint16_t top = compute_pwm_top_and_divider(hz, USE_DUAL_SLOPE, &divider);
    uint16_t match_a = compute_pwm_match_level(top, duty, true);  

    _selected_divider = divider;
    _selected_wrap = top; 
    _selected_level = match_a;

    _left_slice = configure_pwm_slice(GPIO_LEFT_HIGH, GPIO_LEFT_LOW, top, match_a, divider, false);
    _right_slice = configure_pwm_slice(GPIO_RIGHT_HIGH, GPIO_RIGHT_LOW, top, match_a, divider, true);

    _user_wrap_handler = wrap_handler; 

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(_left_slice); 
    pwm_set_irq_enabled(_left_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_wrap_callback);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // both counters are in perfect sync from zero
    pwm_set_counter(_left_slice, 0);
    pwm_set_counter(_right_slice, 0);

    // start both slices simulthaneously  
    pwm_set_mask_enabled(make_both_slices_bitmask()); 
    _slices_are_running = true;
}

/// @brief With running PWM slices change the frequency and the duty cycle.
/// @param hz New frequency
/// @param duty New duty cycle
void mach_pwm_change_waveform(uint hz, float duty)
{
    if(!_slices_are_running) return;

    uint new_divider;
    uint16_t new_top = compute_pwm_top_and_divider(hz, USE_DUAL_SLOPE, &new_divider);
    uint16_t new_match = compute_pwm_match_level(new_top, duty, true);  

    if(new_divider != _selected_divider) {
        pwm_set_clkdiv_int_frac(_left_slice, new_divider, 0); 
        pwm_set_clkdiv_int_frac(_right_slice, new_divider, 0); 
        _selected_divider = new_divider;
    }

    _selected_wrap = new_top;
    _selected_level = new_match; 

    _reloading_wrap_level = true;
    while(_reloading_wrap_level) tight_loop_contents();
}

/// @brief Returns true if PWM is running now.
/// @return True if running, false if not running.
bool mach_pwm_is_running()
{
    return _slices_are_running;
}

/// @brief Stops the PWM signals if running now.
void mach_pwm_stop()
{
    if(_slices_are_running) {
        pwm_set_enabled(_left_slice, false);      
        pwm_set_enabled(_right_slice, false);      
        _slices_are_running = false;

        pwm_set_irq_enabled(_left_slice, false);
        irq_set_enabled(PWM_IRQ_WRAP, false);
        _user_wrap_handler = NULL;

        bring_all_outputs_low(); 
    } 
}

uint16_t compute_pwm_match_level(uint16_t top, float duty, bool inverted)
{
    // Slice produces high output signal when counter is below the match 
    // value and low signal when counter is above the match value. 
    if(!inverted) return (uint16_t)(top * duty / 100);
    return (uint16_t)(top * (100 - duty) / 100);  
}

// For the given frequency and PWM mode returns the TOP/WRAP value, 
// if the divider_ptr is provided stores there the DIVIDER value.
uint16_t compute_pwm_top_and_divider(uint hz, bool dual_slope, uint *divider_ptr)
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

static void bring_output_gpio_low(int gpio)
{
    gpio_init(gpio); // set function GPIO_FUNC_SIO
    gpio_set_pulls(gpio, false, true); // pull-down
    gpio_set_dir(gpio, GPIO_OUT); // output direction
    gpio_put(gpio, false); // low state
}

static void bring_all_outputs_low()
{
    bring_output_gpio_low(GPIO_LEFT_HIGH);
    bring_output_gpio_low(GPIO_LEFT_LOW);
    bring_output_gpio_low(GPIO_RIGHT_HIGH);
    bring_output_gpio_low(GPIO_RIGHT_LOW);
}

