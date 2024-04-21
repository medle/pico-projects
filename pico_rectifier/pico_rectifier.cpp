
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
static void restore_config();
static void save_config();
static void enable_relay(uint relay_gpio, bool on);
static void core1_entry(); 
static void play_beeps(int n);
static void button_callback(uint gpio, bool pressed);
static void repaint_display();

#define CONFIG_MAGIC 0xA000

typedef struct config_t {
    uint magic;
    uint pwm_hz;
    float pwm_duty;
} config_t;

static pico_ssd1306::SSD1306 *_display;
static bool _pwm_running = false;
static bool _mode_hz = true;

static config_t _default_config = 
    { .magic = CONFIG_MAGIC, .pwm_hz = 5000, .pwm_duty = 0.5 };
static config_t _config;

int main() 
{
    stdio_init_all();
    adc_init();
    init_display(); 
    easy_eeprom_init();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    enable_relay(RELAY1_PIN, true); 
    sleep_ms(400); 
    enable_relay(RELAY2_PIN, true); 

    restore_config(); 
    repaint_display();
    multicore_launch_core1(core1_entry);

    int i = 0;

    easy_buttons_register(BUTTON_MODE_PIN, button_callback, true);
    easy_buttons_register(BUTTON_UP_PIN, button_callback, true);
    easy_buttons_register(BUTTON_DOWN_PIN, button_callback, true);
    easy_buttons_register(BUTTON_ON_PIN, button_callback, true);

    while(true) {
        gpio_put(LED_PIN, 1); 
        easy_buttons_sleep_ms(500); 

        gpio_put(LED_PIN, 0); 
        easy_buttons_sleep_ms(500); 
    }

    return 0;
}

static void toggle_pwm_running_state()
{
    _pwm_running = !_pwm_running;
    if (_pwm_running) easy_pwm_enable(PWM_PIN, _config.pwm_hz, _config.pwm_duty);
    else easy_pwm_disable(PWM_PIN);
    repaint_display();
}

static void maybe_update_pwm_waveform()
{
    if (_pwm_running) {
        easy_pwm_disable(PWM_PIN);
        easy_pwm_enable(PWM_PIN, _config.pwm_hz, _config.pwm_duty);
    }
}

static void button_callback(uint gpio, bool pressed)
{
    if (!pressed) {
        if (gpio == BUTTON_UP_PIN || gpio == BUTTON_DOWN_PIN) save_config();
        return;
    }

    int hz_increment = 10;
    float duty_increment = 0.01;

    switch (gpio) {
        case BUTTON_ON_PIN:
            toggle_pwm_running_state();
            break;
        case BUTTON_UP_PIN:
            if (_mode_hz && _config.pwm_hz < 50000) 
                _config.pwm_hz += hz_increment;
            if (!_mode_hz && _config.pwm_duty <= (1 - duty_increment)) 
                _config.pwm_duty += duty_increment;
            repaint_display();
            maybe_update_pwm_waveform();
            break;
        case BUTTON_DOWN_PIN:
            if (_mode_hz && _config.pwm_hz >= 500) 
                _config.pwm_hz -= hz_increment;
            if (!_mode_hz && _config.pwm_duty >= duty_increment) 
                _config.pwm_duty -= duty_increment;
            repaint_display();
            maybe_update_pwm_waveform();
            break;
        case BUTTON_MODE_PIN:
            _mode_hz = !_mode_hz;
            repaint_display();
            break;        
    }
}

static void repaint_display()
{
    int font_width = 12;
    int font_height = 16;

    // when PWM is running display is bright
    pico_ssd1306::WriteMode clear_mode = _pwm_running ?
        pico_ssd1306::WriteMode::ADD : pico_ssd1306::WriteMode::SUBTRACT;
    pico_ssd1306::fillRect(_display, 0, 0, 
        DISPLAY_WIDTH, font_height, clear_mode);

    char buf[50];

    sprintf(buf, "%dHz", _config.pwm_hz);
    pico_ssd1306::drawText(_display, font_12x16, buf, 0, 0,
        pico_ssd1306::WriteMode::INVERT);
    int hz_width = strlen(buf) * font_width;    

    int pwm_percent = (int)(_config.pwm_duty * 100);
    sprintf(buf, "%d%%", pwm_percent);
    int duty_width = strlen(buf) * font_width;
    int duty_x = DISPLAY_WIDTH - duty_width;
    pico_ssd1306::drawText(_display, font_12x16, buf, duty_x, 0,
        pico_ssd1306::WriteMode::INVERT);

    int x1 = hz_width, x2 = duty_x - 1;
    int y1 = 0, y2 = font_height -1;
    if (_mode_hz) {
        pico_ssd1306::drawLine(_display, x1, y1, x2, y2, 
            pico_ssd1306::WriteMode::INVERT);
    } else {
        pico_ssd1306::drawLine(_display, x1, y2, x2, y1, 
            pico_ssd1306::WriteMode::INVERT);
    }

    _display->sendBuffer(); // send buffer to device and show on screen
}

static void init_display()
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

static void enable_relay(uint relay_gpio, bool on)
{
    gpio_init(relay_gpio);
    gpio_set_dir(relay_gpio, GPIO_OUT);
    gpio_put(relay_gpio, on); 
}

static void play_beeps(int n)
{
    for(int i = 0; i < n; i++) { 
      easy_pwm_enable(SOUND_GEN_PIN, SOUND_GEN_HZ, 0.5);  
      sleep_ms(100);
      easy_pwm_disable(SOUND_GEN_PIN);
      sleep_ms(100);
    }
}

static void restore_config()
{
    int ret = easy_eeprom_read_bytes(0, (uint8_t *)&_config, sizeof(_config));
    if (ret != sizeof(_config) || _config.magic != CONFIG_MAGIC) {
        _config = _default_config;
    }
}

static void save_config()
{
    easy_eeprom_write_bytes(0, (uint8_t *)&_config, sizeof(_config));
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
