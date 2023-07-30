//
// DirectDrive inductor mode functions. 
//

#include <time.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include "machine.h"

const uint gpioDrive = 1; // GPIO1 drives power mosfets
const uint gpioSense = 4; // GPIO4 senses current zero-crossing signal

static bool isRunning = false;
static uint runningSliceNum;
static uint maxWavesParameter = 2;
static uint waveCount = 0;

static bool isMeasuring = false;
static uint measureCount = 0;
#define MAX_MEASURES 10
static uint64_t measures[MAX_MEASURES];

static void senseCallbackRoutine(uint gpio, uint32_t events);
static void enableSenseCallback(bool enable);
static void startPwm(int hertz, float duty);
static uint computeMeasuredWaveHz();

void DirectInit()
{
    // init output gpio, no need to init sense gpio
    gpio_init(gpioDrive); // set function GPIO_FUNC_SIO
    gpio_set_pulls(gpioDrive, false, true); // pull-down
    gpio_set_dir(gpioDrive, GPIO_OUT); // output direction
    gpio_put(gpioDrive, false); // low initial state
}

void DirectSetMaxWaves(uint maxWaves)
{
    maxWavesParameter = maxWaves;
}

bool DirectRunAndRespond(int hertz, float duty)
{
    if(isRunning) DirectStop();  

    waveCount = 0;
    isRunning = true;

    isMeasuring = true;
    measureCount = 0;

    led_set(true);
    startPwm(hertz, duty); 
    enableSenseCallback(true); 

    return command_respond_success("DirectDrive started");
}

bool DirectIsRunning()
{
    return isRunning;
}

void DirectStop()
{
    if (isRunning) {
        isRunning = false;
        led_set(false);
        pwm_set_enabled(runningSliceNum, false);
        enableSenseCallback(false);
    }
}

bool DirectStopAndRespond()
{
    DirectStop();

    char buf[50];
    sprintf(buf, "Stopped (measured wave %d Hz)", computeMeasuredWaveHz());
    return command_respond_success(buf);
}

static void startPwm(int hertz, float duty)
{
    // Tell driving GPIO is allocated to the PWM
    gpio_set_function(gpioDrive, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to driving GPIO
    runningSliceNum = pwm_gpio_to_slice_num(gpioDrive);

    uint divider;
    uint16_t top = compute_pwm_top_and_divider(hertz, false, &divider);
    uint16_t match = compute_pwm_match_level(top, duty, false);  

    // Load parameters into slice but not run
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, top);
    pwm_config_set_clkdiv_int(&config, divider);
    pwm_init(runningSliceNum, &config, false); 

    // Set PWM compare values for both A/B channels 
    pwm_set_both_levels(runningSliceNum, match, match);

    // Set the PWM running
    pwm_set_enabled(runningSliceNum, true);
}

static void senseCallbackRoutine(uint gpio, uint32_t events)
{
    waveCount += 1;
    if (waveCount >= maxWavesParameter) {
       pwm_set_counter(runningSliceNum, 0);
       waveCount = 0;
       mach_adc_handle_period_end();
    }

    if (isMeasuring) {
        measures[measureCount] = time_us_64();
        if (++measureCount == MAX_MEASURES) isMeasuring = false;
    }
} 

static void enableSenseCallback(bool enable)
{
    // Schedule a sense routine call on each falling sense signal edge - the
    // moment when LC-tank current goes back to a full stop during oscillating.
    uint flags = /*GPIO_IRQ_EDGE_RISE |*/ GPIO_IRQ_EDGE_FALL;
    gpio_set_irq_enabled_with_callback(gpioSense, flags, enable, &senseCallbackRoutine);
}

static uint computeMeasuredWaveHz()
{
    if (measureCount <= 1) return 0;
    
    uint maxUs = 0;
    for (int i = 1; i < measureCount; i++) {
        uint us = (uint)(measures[i] - measures[i - 1]);
        if (us > maxUs) maxUs = us;
    }

    return maxUs > 0 ? 1000000 / maxUs : 0;
}
