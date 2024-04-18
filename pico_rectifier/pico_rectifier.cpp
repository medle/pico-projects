
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include <ssd1306.h>
#include <ShapeRenderer.h>
#include <TextRenderer.h>

#include "easy_pwm.h"
#include "easy_eeprom.h"
#include "easy_buttons.h"
#include "board_config.h"

static void init_display();
static void enable_relay(uint relay_gpio, bool on);
static void core1_entry(); 

static pico_ssd1306::SSD1306 *display;

static uint n = 0;

static void button_callback(uint gpio, bool on)
{
    if (on) n += 1;
    char buf[50];
    sprintf(buf, "#%d %s", n, (on ? "ON" : "OFF"));  
    pico_ssd1306::fillRect(display, 0, 16, 128, 32, pico_ssd1306::WriteMode::SUBTRACT);
    pico_ssd1306::drawText(display, font_12x16, buf, 0, 16);
    display->sendBuffer(); 
}

int main() 
{
    stdio_init_all();
    adc_init();
    init_display(); 
    easy_eeprom_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BUTTON_1_PIN);
    gpio_set_dir(BUTTON_1_PIN, GPIO_IN);

    enable_relay(RELAY1_PIN, true); 

    for(int j=0; j<2; j++) { 
      easy_pwm_enable(SOUND_GEN_PIN, SOUND_GEN_HZ, 0.5);  
      sleep_ms(100);
      easy_pwm_disable(SOUND_GEN_PIN);
      sleep_ms(100);
    }

    enable_relay(RELAY2_PIN, true); 

    multicore_launch_core1(core1_entry);

    // Main Loop 
    int i = 0;

    //easy_eeprom_read_bytes(0, (uint8_t *)&i, sizeof(int));
    easy_buttons_register(BUTTON_1_PIN, button_callback, true, 75);

    while(true) {
        gpio_put(LED_PIN, 1); 
        easy_buttons_sleep_ms(500); 

        gpio_put(LED_PIN, 0); 
        easy_buttons_sleep_ms(500); 

        // clear line and print a new text  
        pico_ssd1306::fillRect(display, 0, 0, 128, 16, pico_ssd1306::WriteMode::SUBTRACT);

        char buf[50];
        sprintf(buf, "#%d", ++i);  
        pico_ssd1306::drawText(display, font_12x16, buf, 0, 0);
        display->sendBuffer(); 

        //easy_eeprom_write_bytes(0, (uint8_t *)&i, sizeof(int));
    }

    return 0;
}

//
// Core1 procedure runs in parallel to the main loop running in Core0.
//
static void core1_entry() 
{
    while (1) {
        tight_loop_contents();
    }
}

static void init_display()
{
    i2c_init(DISPLAY_I2C_PORT, DISPLAY_I2C_BAUDRATE); 
    gpio_set_function(DISPLAY_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_I2C_SDA_PIN);
    gpio_pull_up(DISPLAY_I2C_SCL_PIN);

    // create a new display object
    display = new pico_ssd1306::SSD1306(
        DISPLAY_I2C_PORT, DISPLAY_I2C_ADDRESS, pico_ssd1306::Size::W128xH32);

    // let the LCD time to initialize itself     
    sleep_ms(200);

    // set flipped orientation    
    display->setOrientation(false);  

    pico_ssd1306::drawText(display, font_12x16, "Rectifier", 0, 0);
    display->sendBuffer(); // send buffer to device and show on screen
}

static void enable_relay(uint relay_gpio, bool on)
{
    gpio_init(relay_gpio);
    gpio_set_dir(relay_gpio, GPIO_OUT);
    gpio_put(relay_gpio, on); 
}
