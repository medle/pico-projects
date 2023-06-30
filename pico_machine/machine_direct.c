
#include "machine.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

const uint gpioDrive = 1; // GPIO1 drives power mosfets
const uint gpioSense = 4; // GPIO4 senses current zero-crossing signal

static void SenseCallback(uint gpio, uint32_t events);
static void EnableSenseCallback(bool enable);
static void StartPwm();
static void StopPwm();

void DirectInit()
{
    // init output gpio
    gpio_init(gpioDrive); // set function GPIO_FUNC_SIO
    gpio_set_pulls(gpioDrive, false, true); // pull-down
    gpio_set_dir(gpioDrive, GPIO_OUT); // output direction
    gpio_put(gpioDrive, false); // low initial state

    // init sense gpio 
    //gpio_init(gpioSense);
    //gpio_set_dir(gpioSense, GPIO_IN);
}

static uint senseCount;
static bool isRunning = false;

bool DirectRunAndRespond()
{
    led_set(true);

    senseCount = 0;
    isRunning = true;

    StartPwm(); 
    EnableSenseCallback(true); 

    return command_respond_success("DirectDrive2 started");
}

void DirectStop()
{
    led_set(false);
    StopPwm();
    isRunning = false;
    EnableSenseCallback(false);
}

static void StartPwm()
{
    uint hz = 5600;
    uint duty = 8; 

    // Tell GPIO 0 and 1 they are allocated to the PWM
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(1, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    uint slice_num = pwm_gpio_to_slice_num(0);

    uint divider;
    uint16_t top = compute_pwm_top_and_divider(hz, false, &divider);
    uint16_t match = compute_pwm_match_level(top, duty, false);  

    // load parameters into slice but not run
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, top);
    pwm_config_set_clkdiv_int(&config, divider);
    pwm_init(slice_num, &config, false); 

    // set PWM compare values for both A/B channels 
    pwm_set_both_levels(slice_num, match, match);

    // Set the PWM running
    pwm_set_enabled(slice_num, true);
}

static void StopPwm()
{
    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    uint slice_num = pwm_gpio_to_slice_num(0);
    pwm_set_enabled(slice_num, false);
}

static void SenseCallback(uint gpio, uint32_t events)
{
    senseCount += 1;
    if (senseCount % 3 == 0) {
       uint slice_num = pwm_gpio_to_slice_num(0);
       pwm_set_counter(slice_num, 0);
    }
} 

static void EnableSenseCallback(bool enable)
{
    gpio_set_irq_enabled_with_callback(
        gpioSense, /*GPIO_IRQ_EDGE_RISE |*/ GPIO_IRQ_EDGE_FALL, enable, &SenseCallback);
}

