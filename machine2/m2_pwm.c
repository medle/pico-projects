
#include "m2_global.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static bool _isRunning = false;
static uint _sliceNum;
static void (*_userWrapHandler)();

static void bringGpioLow(int gpio);
static void bringOutputsLow();
static void onPwmWrapCallback();

void machPwmInit()
{
    bringOutputsLow();

    // figure out which PWM slice the gpio is connected to
    _sliceNum = pwm_gpio_to_slice_num(MACH_PWM_GPIO_A);    
}

bool machPwmStart(uint hz, float duty, void (*wrapHandler)())
{
    assert(duty >= 0 && duty <= 100);
    if (_isRunning) machPwmStop();

    _userWrapHandler = wrapHandler; 

    gpio_set_function(MACH_PWM_GPIO_A, GPIO_FUNC_PWM);
    gpio_set_function(MACH_PWM_GPIO_B, GPIO_FUNC_PWM);

    const bool dualSlope = false;
    pwm_config config = pwm_get_default_config();
    PwmTopDivider topDivider = pwmChooseTopDivider(hz, dualSlope); 
    pwm_config_set_phase_correct(&config, dualSlope);
    pwm_config_set_wrap(&config, topDivider.top);
    pwm_config_set_clkdiv_int(&config, topDivider.divider);

    // channel A is not inverted, channel B is inverted 
    pwm_config_set_output_polarity(&config, false, true);

    // load into slice but not run
    pwm_init(_sliceNum, &config, false);

    // set PWM compare values for both A/B channels 
    uint highCycles = (uint)(topDivider.top * duty);
    pwm_set_both_levels(_sliceNum, highCycles, highCycles);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(_sliceNum); 
    pwm_set_irq_enabled(_sliceNum, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, onPwmWrapCallback);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // reset the counter value
    pwm_set_counter(_sliceNum, 0);

    // start PWM
    pwm_set_enabled(_sliceNum, true);
    _isRunning = true;
    return true;
}

void machPwmResetCounter()
{
    if (_isRunning) pwm_set_counter(_sliceNum, 0);
}

bool machPwmStop()
{
    if (!_isRunning) return false;
    pwm_set_enabled(_sliceNum, false);

    // disable the wrap interrupt handler 
    pwm_set_irq_enabled(_sliceNum, false);
    irq_set_enabled(PWM_IRQ_WRAP, false);

    bringOutputsLow();
    _isRunning = false;
    return true;
}

static void onPwmWrapCallback()
{
    // determine which slice has caused the irq:
    uint32_t mask = pwm_get_irq_status_mask();
    if(mask & (1 << _sliceNum)) {
        pwm_clear_irq(_sliceNum);
        if (_userWrapHandler != NULL) _userWrapHandler();
    }
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

static void bringOutputsLow()
{
    bringGpioLow(MACH_PWM_GPIO_A);
    bringGpioLow(MACH_PWM_GPIO_B);
}

static void bringGpioLow(int gpio)
{
    gpio_init(gpio); // set function GPIO_FUNC_SIO
    gpio_set_pulls(gpio, false, true); // pull-down
    gpio_set_dir(gpio, GPIO_OUT); // output direction
    gpio_put(gpio, false); // low state
}