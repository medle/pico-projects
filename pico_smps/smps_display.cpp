
#include "smps_globals.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include <ssd1306.h>
#include <ShapeRenderer.h>
#include <TextRenderer.h>

static pico_ssd1306::SSD1306 *_display;

static void draw_arrow(pico_ssd1306::SSD1306 *display,
    int x, int y, int length, int angle, 
    pico_ssd1306::WriteMode write_mode);

void smps_display_init()
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
    _display->setOrientation(false);  
}

void smps_display_repaint()
{
    const unsigned char *font_ptr = font_12x16;
    int font_width = 12;
    int font_height = 16;

    // when PWM is running display is bright
    pico_ssd1306::WriteMode clear_mode = _smps_pwm_running ?
        pico_ssd1306::WriteMode::ADD : pico_ssd1306::WriteMode::SUBTRACT;
    pico_ssd1306::fillRect(_display, 0, 0, 
        DISPLAY_WIDTH - 1, font_height, clear_mode);

    char buf[50];

    sprintf(buf, "%dHz", _smps_config.pwm_hz);
    pico_ssd1306::drawText(_display, font_ptr, buf, 0, 0,
        pico_ssd1306::WriteMode::INVERT);
    int hz_width = strlen(buf) * font_width;    

    int pwm_percent = (int)(_smps_config.pwm_duty * 100);
    sprintf(buf, "%d%%", pwm_percent);
    int duty_width = strlen(buf) * font_width;
    int duty_x = DISPLAY_WIDTH - duty_width;
    pico_ssd1306::drawText(_display, font_ptr, buf, duty_x, 0,
        pico_ssd1306::WriteMode::INVERT);

    int x1 = hz_width, x2 = duty_x - 1;
    int y1 = 0, y2 = font_height -1;
    int angle = 0;
    if (_smps_mode == SMPS_MODE_HZ) angle = 180;
    if (_smps_mode == SMPS_MODE_DUTY) angle = 0;
    if (_smps_mode == SMPS_MODE_LIMIT) angle = 270;
    draw_arrow(_display, (x2 + x1) / 2, (y2 + y1) / 2, 8, angle, 
        pico_ssd1306::WriteMode::INVERT);   

    pico_ssd1306::WriteMode power_mode = pico_ssd1306::WriteMode::ADD;

    int line_2_y = font_height; 
    pico_ssd1306::fillRect(_display, 0, line_2_y, 
        DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, 
        power_mode);

    sprintf(buf, "A%.2f", smps_current_sensor_get_amps());
    pico_ssd1306::drawText(_display, font_ptr, buf, 0, line_2_y,
        pico_ssd1306::WriteMode::INVERT);

    //sprintf(buf, "L%d", (int)_config.amp_limit);
    sprintf(buf, "N%d", _smps_cycle_count);
    int limit_x = DISPLAY_WIDTH - strlen(buf) * font_width;
    pico_ssd1306::drawText(_display, font_ptr, buf, limit_x, line_2_y,
        pico_ssd1306::WriteMode::INVERT);

    _display->sendBuffer(); // send buffer to device and show on screen
}

#define SWAP(Type, v1, v2) { Type v = v1; v1 = v2; v2 = v; }

static void draw_arrow(
    pico_ssd1306::SSD1306 *display_ptr,
    int x, int y, int length, int angle, 
    pico_ssd1306::WriteMode write_mode)
{
    int h = length / 2;
    int x_path[] = { -h, h, 0, h, 0,  h };    
    int y_path[] = {  0, 0, 4, 0, -4, 0 };
    int num_points = 6;
    int num_lines = 4;

    if (angle == 180) {
        for (int i = 0; i < num_points; i++) x_path[i] = -x_path[i];
    }
    if (angle == 90 || angle == 270) {
        for (int i = 0; i < num_points; i++) SWAP(int, x_path[i], y_path[i]);
    }
    if (angle == 90) {
        for (int i = 0; i < num_points; i++) y_path[i] = -y_path[i];
    }

    for (int i = 0; i < num_points; i += 2) {
        pico_ssd1306::drawLine(display_ptr, 
            x + x_path[i], y + y_path[i],
            x + x_path[i + 1], y + y_path[i + 1], 
            write_mode);
    }
}
