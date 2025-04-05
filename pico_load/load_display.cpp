
#include "load.h"

#include <hardware/i2c.h>
#include <hardware/gpio.h>

#include <ssd1306.h>
#include <ShapeRenderer.h>
#include <TextRenderer.h>

static pico_ssd1306::SSD1306 *_display;

void board_display_init()
{
    i2c_init(DISPLAY_I2C_PORT, DISPLAY_I2C_BAUDRATE); 
    gpio_set_function(DISPLAY_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_I2C_SDA_PIN);
    gpio_pull_up(DISPLAY_I2C_SCL_PIN);

    // create a new display object
    _display = new pico_ssd1306::SSD1306(
        DISPLAY_I2C_PORT, DISPLAY_I2C_ADDRESS, pico_ssd1306::Size::W128xH32);

    // let the LCD time to initialize itself     
    sleep_ms(200);

    // set flipped orientation    
    _display->setOrientation(true);  
}

void board_display_repaint()
{
    const unsigned char *font_ptr = font_12x16;
    int font_width = 12;
    int font_height = 16;

    pico_ssd1306::WriteMode clear_mode = pico_ssd1306::WriteMode::SUBTRACT;
    pico_ssd1306::WriteMode invert_mode = pico_ssd1306::WriteMode::INVERT; 

    pico_ssd1306::fillRect(_display, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, clear_mode);

    char buf[50];
    sprintf(buf, "encoder");
    pico_ssd1306::drawText(_display, font_ptr, buf, 0, 0, invert_mode);

    pico_ssd1306::drawText(_display, font_ptr, g_buffer, 0, font_height, invert_mode);

    _display->sendBuffer(); // send buffer to device and show on screen
}

