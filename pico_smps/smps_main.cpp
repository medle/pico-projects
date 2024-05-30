
#include "smps.h"

#include "easy_pwm.h"
#include "easy_buttons.h"

static void button_callback(uint gpio, bool pressed);
static void enable_relay(uint relay_gpio, bool on);
static void enable_all_relays(bool on);

// Global state.
bool _smps_pwm_running = false;
int _smps_mode = SMPS_MODE_HZ;
volatile bool _smps_limit_occured = false;
bool _smps_alarm_occured = false;
int _smps_cycle_count = 0;

int main() 
{
    stdio_init_all();

    smps_led_init(); 
    smps_display_init(); 
    smps_memory_init();
    smps_memory_restore();
    smps_current_sensor_init();
    smps_pio_start_repeater();  
    smps_display_repaint();

    easy_buttons_register(BUTTON_MODE_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_UP_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_DOWN_PIN, button_callback, true, true);
    easy_buttons_register(BUTTON_ON_PIN, button_callback, true, false);

    const int cycle_ms = 500; 
  
    while(true) {
        if (_smps_limit_occured) smps_sound_enable(true);
        smps_led_set(true); 
        easy_buttons_sleep_ms(cycle_ms / 2); 
        if (_smps_limit_occured) smps_sound_enable(false);
        _smps_limit_occured = false;  

        smps_led_set(false); 
        easy_buttons_sleep_ms(cycle_ms / 2);

        smps_display_repaint();
        _smps_cycle_count += 1;
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

static void toggle_pwm_running_state()
{
    _smps_pwm_running = !_smps_pwm_running;
    if (_smps_pwm_running) {
        easy_pwm_enable(PWM_PIN, _smps_memory.pwm_hz, _smps_memory.pwm_duty);
    }
    else {
        easy_pwm_disable(PWM_PIN);
        _smps_alarm_occured = false;
    }

    smps_display_repaint();

    enable_all_relays(_smps_pwm_running);
}

static void maybe_update_pwm_waveform()
{
    if (_smps_pwm_running) {
        easy_pwm_disable(PWM_PIN);
        easy_pwm_enable(PWM_PIN, _smps_memory.pwm_hz, _smps_memory.pwm_duty);
    }
}

static void button_callback(uint gpio, bool pressed)
{
    if (!pressed) {
        if (gpio == BUTTON_UP_PIN || gpio == BUTTON_DOWN_PIN) smps_memory_save();
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
            if (_smps_mode == SMPS_MODE_HZ && _smps_memory.pwm_hz < 50000) 
                _smps_memory.pwm_hz += hz_increment;
            if (_smps_mode == SMPS_MODE_DUTY && _smps_memory.pwm_duty <= (1 - duty_increment)) 
                _smps_memory.pwm_duty += duty_increment;
            if (_smps_mode == SMPS_MODE_LIMIT && _smps_memory.amp_limit <= 
                (CURRENT_SENSOR_MAX_AMPS - amp_increment))
                _smps_memory.amp_limit += amp_increment;    
            smps_display_repaint();
            maybe_update_pwm_waveform();
            break;

        case BUTTON_DOWN_PIN:
            if (_smps_mode == SMPS_MODE_HZ && _smps_memory.pwm_hz >= 500) 
                _smps_memory.pwm_hz -= hz_increment;
            if (_smps_mode == SMPS_MODE_DUTY && _smps_memory.pwm_duty >= duty_increment) 
                _smps_memory.pwm_duty -= duty_increment;
            if (_smps_mode == SMPS_MODE_LIMIT && _smps_memory.amp_limit >= amp_increment)
                _smps_memory.amp_limit -= amp_increment;    
            smps_display_repaint();
            maybe_update_pwm_waveform();
            break;

        case BUTTON_MODE_PIN:
            if (_smps_mode == SMPS_MODE_HZ) _smps_mode = SMPS_MODE_DUTY;
            else if (_smps_mode == SMPS_MODE_DUTY) _smps_mode = SMPS_MODE_LIMIT;
            else _smps_mode = SMPS_MODE_HZ;
            smps_display_repaint();
            break;        
    }
}

void smps_enter_alarm_mode()
{
    enable_all_relays(false);
    _smps_alarm_occured = true;
}

static void enable_relay(uint relay_gpio, bool on)
{
    gpio_init(relay_gpio);
    gpio_set_dir(relay_gpio, GPIO_OUT);
    gpio_put(relay_gpio, on); 
}


