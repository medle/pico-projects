
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

static void configure_pwm(uint gpio)
{
    uint hz = 8300;
    float duty = 0.25;

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