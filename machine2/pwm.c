
#include "machine.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static bool isRunning = false;
static uint sliceNum;

void pwmInit(uint gpio)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    gpio_set_function(gpio + 1, GPIO_FUNC_PWM);

    // figure out which PWM slice the gpio is connected to
    sliceNum = pwm_gpio_to_slice_num(gpio);    
}

void pwmStart(uint hz, float duty)
{
    if (isRunning) pwmStop();

    const bool dualSlope = false;
    pwm_config config = pwm_get_default_config();
    PwmTopDivider topDivider = pwmChooseTopDivider(hz, dualSlope); 
    pwm_config_set_phase_correct(&config, dualSlope);
    pwm_config_set_wrap(&config, topDivider.top);
    pwm_config_set_clkdiv_int(&config, topDivider.divider);

    // channel A is not inverted, channel B is inverted 
    pwm_config_set_output_polarity(&config, false, true);

    // load into slice but not run
    pwm_init(sliceNum, &config, false);

    // set PWM compare values for both A/B channels 
    uint highCycles = (uint)(topDivider.top * duty);
    pwm_set_both_levels(sliceNum, highCycles, highCycles);

    // reset the counter value
    pwm_set_counter(sliceNum, 0);

    // start PWM
    pwm_set_enabled(sliceNum, true);
    isRunning = true;
}

void pwmStop()
{
    if (!isRunning) return;
    pwm_set_enabled(sliceNum, false);
    isRunning = false;
}

PwmTopDivider pwmChooseTopDivider(uint periodsPerSecond, bool dualSlope)
{
    PwmTopDivider result;
    const uint16_t maxTop = 0xFFFF;
    const uint16_t maxDivider = 256;
    uint32_t clocksPerSecond = clock_get_hz(clk_sys);

    uint topsPerSecond = dualSlope ? (2 * periodsPerSecond) : periodsPerSecond;

    uint16_t divider = 1;
    for (; divider <= maxDivider; divider++) {
        // minus one cycle by RP2040 datasheet
        uint16_t top = clocksPerSecond / (divider * topsPerSecond) - 1;
        if (top <= maxTop) {
            result.top = top;
            result.divider = divider;
            return result;  
        }     
    }

    result.top = maxTop;
    result.divider = maxDivider; 
    return result;
}
