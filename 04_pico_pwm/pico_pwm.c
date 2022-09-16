

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static bool led_init_done = 0;
static void set_led(bool on);
static void run_startup_led_welcome();
static void configure_pwm();

int main() 
{
    stdio_init_all(); 
    run_startup_led_welcome();

    sleep_ms(1000);

    printf("starting\n"); 
    uint32_t hz = clock_get_hz(clk_sys);
    printf("clock_get_hz(clk_sys)=%d\n", hz); // 125MHz

    configure_pwm(0);

    while (true) {
        sleep_ms(1000);
        set_led(1);
        sleep_ms(5);
        set_led(0);
    }
}

static void configure_pwm(uint gpio)
{
    uint hz = 8300;
    float duty = 0.25;

    gpio_set_function(gpio, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();

    // set period [0,count_top] inclusive
    uint count_top = 10000; //clk_sys / hz;
    pwm_config_set_wrap(&config, count_top);

    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, false);

    uint high_cycles = (uint)(count_top * duty);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, high_cycles);

    pwm_set_enabled(slice_num, true);
}

static void run_startup_led_welcome()
{
    for(int i = 0; i < 2; i++) {
        set_led(1);
        sleep_ms(100);
        set_led(0);
        sleep_ms(100);
    }
}

static void set_led(bool on)
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    if(!led_init_done) {
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        led_init_done = true;
    }

   gpio_put(LED_PIN, on);
}