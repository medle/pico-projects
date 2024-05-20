
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
static void init_current_sensor();
static void restore_config();
static void save_config();
static void enable_relay(uint relay_gpio, bool on);
static void core1_entry(); 
static void play_beeps(int n);
static void button_callback(uint gpio, bool pressed);
static void power_input_callback(uint gpio, bool pressed);
static void repaint_display();
static void enable_all_relays(bool on);
static void enable_sound(bool enable);
static void draw_arrow(pico_ssd1306::SSD1306 *display,
    int x, int y, int length, int angle, 
    pico_ssd1306::WriteMode write_mode);
static float convert_adc_to_amps(uint8_t adc_reading);    
static uint8_t get_adc_sample();

#define CONFIG_MAGIC 0xB000

typedef struct config_t {
    uint magic;
    uint pwm_hz;
    float pwm_duty;
    volatile float amp_limit;
} config_t;

static config_t _default_config = { 
    .magic = CONFIG_MAGIC, 
    .pwm_hz = 5000, 
    .pwm_duty = 0.5,
    .amp_limit = 5
};

static config_t _config;

#define MODE_HZ 0
#define MODE_DUTY 1
#define MODE_LIMIT 2

static pico_ssd1306::SSD1306 *_display;
static bool _pwm_running = false;
static int _mode = MODE_HZ;

// Pi pico ADC sample rate is 600K per second,
// so 100*100=10K and 60 measures per second
static uint8_t _adc_array_level_1[100];
static uint8_t _adc_array_level_2[100];
static int _adc_index_level_1 = 0;
static int _adc_index_level_2 = 0;
static int _overcurrent_count = 0;

static int _power_on_count = 0;
static int _power_off_count = 0;

static bool _alarm_occured = false;
static int _cycle_count = 0;


int main() 
{
    stdio_init_all();
    adc_init();
    init_display(); 
    easy_eeprom_init();
    init_current_sensor();
    restore_config();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    multicore_launch_core1(core1_entry);
    repaint_display();

    //easy_buttons_register(POWER_INPUT_PIN, power_input_callback, false, false);
    easy_buttons_register(BUTTON_MODE_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_UP_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_DOWN_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_ON_PIN, button_callback, true, false);

    const int cycle_ms = 500; 

    while(true) {
        //if (_alarm_occured) enable_sound(true);
        gpio_put(LED_PIN, 1); 
        easy_buttons_sleep_ms(cycle_ms / 2); 
        //if (_alarm_occured) enable_sound(false);

        gpio_put(LED_PIN, 0); 
        easy_buttons_sleep_ms(cycle_ms / 2);

        repaint_display();
        _cycle_count += 1;
    }

    return 0;
}

static void enable_all_relays(bool on)
{
    if (on) {
        enable_relay(RELAY1_PIN, true); 
        sleep_ms(500); 
        enable_relay(RELAY2_PIN, true); 
    } else {
        enable_relay(RELAY1_PIN, false);
        enable_relay(RELAY2_PIN, false);
    }
}

static void power_input_callback(uint gpio, bool pressed)
{
    if (pressed) { // power appears
        _power_on_count += 1;
        if (!_alarm_occured) enable_all_relays(true);
    }
    else { // power disappears
        _power_off_count += 1;
        enable_all_relays(false); 
        _alarm_occured = false;
    }
    repaint_display();
}

static void toggle_pwm_running_state()
{
    _pwm_running = !_pwm_running;
    if (_pwm_running) {
        easy_pwm_enable(PWM_PIN, _config.pwm_hz, _config.pwm_duty);
    }
    else {
        easy_pwm_disable(PWM_PIN);
    }

    repaint_display();

    enable_all_relays(_pwm_running);
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
    float amp_increment = 1;

    switch (gpio) {

        case BUTTON_ON_PIN:
            toggle_pwm_running_state();
            break;

        case BUTTON_UP_PIN:
            if (_mode == MODE_HZ && _config.pwm_hz < 50000) 
                _config.pwm_hz += hz_increment;
            if (_mode == MODE_DUTY && _config.pwm_duty <= (1 - duty_increment)) 
                _config.pwm_duty += duty_increment;
            if (_mode == MODE_LIMIT && _config.amp_limit <= 
                (CURRENT_SENSOR_MAX_AMPS - amp_increment))
                _config.amp_limit += amp_increment;    
            repaint_display();
            maybe_update_pwm_waveform();
            break;

        case BUTTON_DOWN_PIN:
            if (_mode == MODE_HZ && _config.pwm_hz >= 500) 
                _config.pwm_hz -= hz_increment;
            if (_mode == MODE_DUTY && _config.pwm_duty >= duty_increment) 
                _config.pwm_duty -= duty_increment;
            if (_mode == MODE_LIMIT && _config.amp_limit >= amp_increment)
                _config.amp_limit -= amp_increment;    
            repaint_display();
            maybe_update_pwm_waveform();
            break;

        case BUTTON_MODE_PIN:
            if (_mode == MODE_HZ) _mode = MODE_DUTY;
            else if (_mode == MODE_DUTY) _mode = MODE_LIMIT;
            else _mode = MODE_HZ;
            repaint_display();
            break;        
    }
}

static void repaint_display()
{
    const unsigned char *font_ptr = font_12x16;
    int font_width = 12;
    int font_height = 16;

    // when PWM is running display is bright
    pico_ssd1306::WriteMode clear_mode = _pwm_running ?
        pico_ssd1306::WriteMode::ADD : pico_ssd1306::WriteMode::SUBTRACT;
    pico_ssd1306::fillRect(_display, 0, 0, 
        DISPLAY_WIDTH - 1, font_height, clear_mode);

    char buf[50];

    sprintf(buf, "%dHz", _config.pwm_hz);
    pico_ssd1306::drawText(_display, font_ptr, buf, 0, 0,
        pico_ssd1306::WriteMode::INVERT);
    int hz_width = strlen(buf) * font_width;    

    int pwm_percent = (int)(_config.pwm_duty * 100);
    sprintf(buf, "%d%%", pwm_percent);
    int duty_width = strlen(buf) * font_width;
    int duty_x = DISPLAY_WIDTH - duty_width;
    pico_ssd1306::drawText(_display, font_ptr, buf, duty_x, 0,
        pico_ssd1306::WriteMode::INVERT);

    int x1 = hz_width, x2 = duty_x - 1;
    int y1 = 0, y2 = font_height -1;
    int angle = 0;
    if (_mode == MODE_HZ) angle = 180;
    if (_mode == MODE_DUTY) angle = 0;
    if (_mode == MODE_LIMIT) angle = 270;
    draw_arrow(_display, (x2 + x1) / 2, (y2 + y1) / 2, 8, angle, 
        pico_ssd1306::WriteMode::INVERT);   

    pico_ssd1306::WriteMode power_mode = _power_on_count > _power_off_count ?
        pico_ssd1306::WriteMode::ADD : pico_ssd1306::WriteMode::SUBTRACT;

    int line_2_y = font_height; 
    pico_ssd1306::fillRect(_display, 0, line_2_y, 
        DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, 
        power_mode);

    sprintf(buf, "A%.1f", convert_adc_to_amps(get_adc_sample()));
    pico_ssd1306::drawText(_display, font_ptr, buf, 0, line_2_y,
        pico_ssd1306::WriteMode::INVERT);

    //sprintf(buf, "L%d", (int)_config.amp_limit);
    sprintf(buf, "N%d", _cycle_count);
    int limit_x = DISPLAY_WIDTH - strlen(buf) * font_width;
    pico_ssd1306::drawText(_display, font_ptr, buf, limit_x, line_2_y,
        pico_ssd1306::WriteMode::INVERT);

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

static void enter_alarm_mode()
{
    enable_all_relays(false);
    _alarm_occured = true;
}

static void enable_relay(uint relay_gpio, bool on)
{
    gpio_init(relay_gpio);
    gpio_set_dir(relay_gpio, GPIO_OUT);
    gpio_put(relay_gpio, on); 
}

static void enable_sound(bool enable)
{
    if (enable) easy_pwm_enable(SOUND_GEN_PIN, SOUND_GEN_HZ, 0.5);  
    else easy_pwm_disable(SOUND_GEN_PIN);
}

static void play_beeps(int n)
{
    for(int i = 0; i < n; i++) {
      enable_sound(true);   
      sleep_ms(100);
      enable_sound(false);
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

static void init_current_sensor()
{
    adc_gpio_init(CURRENT_SENSOR_ADC_PIN);
    adc_select_input(CURRENT_SENSOR_ADC_PIN - FIRST_ADC_PIN);

    memset(_adc_array_level_1, CURRENT_SENSOR_ZERO_READING, sizeof(_adc_array_level_1));
    memset(_adc_array_level_2, CURRENT_SENSOR_ZERO_READING, sizeof(_adc_array_level_2));
}

static uint8_t compute_average(uint8_t *array, uint length)
{
    uint sum = 0;
    for(int i = 0; i < length; i++) sum += array[i];
    return (uint8_t)(sum / length);
}

static uint8_t get_adc_sample()
{
    return compute_average(_adc_array_level_2, sizeof(_adc_array_level_2));
}

//
// Core1 procedure runs in parallel to the main loop running in Core0.
//
static void core1_entry() 
{
    while (1) {
        uint16_t value12bit = adc_read();
        uint8_t sample = value12bit >> 4;
        _adc_array_level_1[_adc_index_level_1] = sample;

        if(++_adc_index_level_1 == sizeof(_adc_array_level_1)) {
            _adc_index_level_1 = 0;
            
            uint8_t avg1 = compute_average(_adc_array_level_1, sizeof(_adc_array_level_1));
            _adc_array_level_2[_adc_index_level_2] = avg1;
            if (++_adc_index_level_2 == sizeof(_adc_array_level_2)) { 
              _adc_index_level_2 = 0;

                // at the end of each buffer cycle check amp limit
                float amps = convert_adc_to_amps(get_adc_sample());
                if (abs(amps) > _config.amp_limit) {
                    enter_alarm_mode();
                    _overcurrent_count += 1;
                }
            }
        }
    }
}

static float convert_adc_to_amps(uint8_t adc_reading)
{
    bool reversed = false;
    if (adc_reading < CURRENT_SENSOR_ZERO_READING) {
        reversed = true;
        adc_reading = CURRENT_SENSOR_ZERO_READING +
            (CURRENT_SENSOR_ZERO_READING - adc_reading);
    }
      
    if (adc_reading > CURRENT_SENSOR_MAX_READING)
        adc_reading = CURRENT_SENSOR_MAX_READING;

    int delta = adc_reading - CURRENT_SENSOR_ZERO_READING;
    int span = CURRENT_SENSOR_MAX_READING - CURRENT_SENSOR_ZERO_READING;
    float rate = (float)delta / span;
    float amps = rate * CURRENT_SENSOR_MAX_AMPS;
    return amps * (reversed ? -1 : 1);
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

