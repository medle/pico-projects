
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include <ssd1306.h>
#include <ShapeRenderer.h>
#include <TextRenderer.h>

#define LCD_I2C_PORT i2c0
#define LCD_I2C_ADDRESS 0x3C
#define LCD_I2C_BAUDRATE 1000000
#define LCD_I2C_SDA_PIN 20
#define LCD_I2C_SCL_PIN 21

static void enable_pwm(uint gpio, uint hz, float duty);
static void disable_pwm(uint gpio);

int main() {
    stdio_init_all();

    // initialise GPIO (Green LED connected to pin 25)
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    i2c_init(LCD_I2C_PORT, LCD_I2C_BAUDRATE); 
    gpio_set_function(LCD_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_I2C_SDA_PIN);
    gpio_pull_up(LCD_I2C_SCL_PIN);

    // Create a new display object
    pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(
        LCD_I2C_PORT, LCD_I2C_ADDRESS, pico_ssd1306::Size::W128xH32);

    // let the LCD time to initialize itself     
    sleep_ms(300);

    pico_ssd1306::drawText(&display, font_12x16, "Rectifier", 0, 0);
    display.sendBuffer(); //Send buffer to device and show on screen

    enable_pwm(14, 6000, 0.5);  
    sleep_ms(500);
    disable_pwm(14);

    // Main Loop 
    int i = 0;
    while(true) {
        gpio_put(25, 1); // Set pin 25 to high
        sleep_ms(500); 

        gpio_put(25, 0); // Set pin 25 to low
        sleep_ms(500); 

        char buf[50];
        sprintf(buf, "N=%d\n", ++i);  
 
        // clear line and print a new text  
        pico_ssd1306::fillRect(&display, 0, 16, 128, 32, pico_ssd1306::WriteMode::SUBTRACT);
        pico_ssd1306::drawText(&display, font_12x16, buf, 0, 16);
        display.sendBuffer(); 
    }

    return 0;
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

static void enable_pwm(uint gpio, uint hz, float duty)
{
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

static void disable_pwm(uint gpio)
{
    // figure out which slice we just connected to the pin
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice_num, false);
    gpio_set_function(gpio, GPIO_FUNC_NULL);
}
