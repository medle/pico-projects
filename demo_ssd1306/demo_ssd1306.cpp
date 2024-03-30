
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <ssd1306.h>
#include <ShapeRenderer.h>
#include <TextRenderer.h>

int main() {
    stdio_init_all();

    // initialise GPIO (Green LED connected to pin 25)
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    sleep_ms(1000);
    printf("Starting...\n");

    i2c_inst_t *i2c_port = i2c0; 

    i2c_init(i2c_port, 1000000); //Use i2c port with baud rate of 1Mhz
    //Set pins for I2C operation
    gpio_set_function(20, GPIO_FUNC_I2C);
    gpio_set_function(21, GPIO_FUNC_I2C);
    gpio_pull_up(20);
    gpio_pull_up(21);
    
    sleep_ms(300);

    // Create a new display object
    uint16_t address = 0x3C;
    pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(i2c_port, address, pico_ssd1306::Size::W128xH32);
    sleep_ms(300);

    pico_ssd1306::drawLine(&display, 0, 16, 128, 32);
    pico_ssd1306::drawText(&display, font_12x16, "SSD1306I2C", 0, 0);
    display.sendBuffer(); //Send buffer to device and show on screen

    // Main Loop 
    int i = 0;
    while(true) {
        gpio_put(25, 1); // Set pin 25 to high
        sleep_ms(500); 

        gpio_put(25, 0); // Set pin 25 to low
        sleep_ms(500); 

        char buf[50];
        sprintf(buf, "N=%d\n", ++i);  
        printf(buf);

        // clear line and print a new text  
        pico_ssd1306::fillRect(&display, 0, 16, 128, 32, pico_ssd1306::WriteMode::SUBTRACT);
        pico_ssd1306::drawText(&display, font_12x16, buf, 0, 16);
        display.sendBuffer(); 
    }

    return 0;
}