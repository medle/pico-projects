
#include "hardware/gpio.h"
#include "easy_buttons.h"
#include "time.h"

#define STATE_INITIAL 0
#define STATE_STABLE 2
#define STATE_DEBOUNCE 1

#define DEBOUNCE_US 10000

typedef struct button_t {
    bool enabled;
    uint gpio;
    bool last_stable_value;
    uint64_t debounce_start_us;
    uint8_t state;
    void (*callback)(uint, bool);
    bool released_value;
    uint repeat_ms;
    uint64_t repeat_start_us;
} button_t;

#define MAX_BUTTONS 4
static button_t s_buttons[MAX_BUTTONS];

static inline bool is_pressed_value(button_t *button_ptr, bool gpio_value)
{
    return (gpio_value != button_ptr->released_value);
}

static inline bool translate_gpio_value_to_button_state(button_t *button_ptr, bool gpio_value)
{
    return (button_ptr->released_value) ? !gpio_value : gpio_value;
}

static void invoke_button_callback(button_t *button_ptr, bool from_value, bool to_value)
{
    if (from_value == to_value) {
        button_ptr->callback(button_ptr->gpio, translate_gpio_value_to_button_state(button_ptr, !from_value));
        button_ptr->callback(button_ptr->gpio, translate_gpio_value_to_button_state(button_ptr, to_value));
    } else { // from_value != to_value
        bool rising = to_value;
        button_ptr->callback(button_ptr->gpio, translate_gpio_value_to_button_state(button_ptr, rising));
    }
}

static void poll_button(button_t *button_ptr, uint64_t now_us)
{
    bool on = gpio_get(button_ptr->gpio);
    switch(button_ptr->state) {

        case STATE_INITIAL: 
            button_ptr->last_stable_value = on;
            button_ptr->state = STATE_STABLE;
            button_ptr->repeat_start_us = 0;
            break;

        case STATE_STABLE:
            if (button_ptr->last_stable_value != on) {
                button_ptr->state = STATE_DEBOUNCE;
                button_ptr->debounce_start_us = time_us_64();
                button_ptr->repeat_start_us = 0;
            }    
            else {
                // if stable pressed state
                if (is_pressed_value(button_ptr, on)) {
                    uint64_t now_us = time_us_64();
                    if (button_ptr->repeat_start_us == 0) button_ptr->repeat_start_us = now_us;
                    else {
                        uint elapsed_ms = (uint)((now_us - button_ptr->repeat_start_us) / 1000);
                        if (elapsed_ms > button_ptr->repeat_ms) {
                            button_ptr->repeat_start_us = now_us;
                            button_ptr->callback(button_ptr->gpio, true);
                        }
                    }
                } else { // stable released state
                    if (button_ptr->repeat_start_us != 0) button_ptr->repeat_start_us = 0;
                }
            }
            break;

        case STATE_DEBOUNCE: 
            if (now_us - button_ptr->debounce_start_us >= DEBOUNCE_US) {
                invoke_button_callback(button_ptr, button_ptr->last_stable_value, on);    
                button_ptr->last_stable_value = on;
                button_ptr->state = STATE_STABLE;
            }
            break;    
    }
}

void easy_buttons_sleep_ms(uint delay_ms)
{
    uint64_t start_us = time_us_64();
    for(;;) {
        uint64_t now_us = time_us_64();
        uint elapsed_ms = (uint)((now_us - start_us) / 1000);
        if (elapsed_ms >= delay_ms) break;

        for(int i = 0; i < MAX_BUTTONS; i++) {
            if (s_buttons[i].enabled) poll_button(&s_buttons[i], now_us);
        }  
    }
}

bool easy_buttons_register(
    uint gpio, void (*callback)(uint, bool), bool released_value, uint repeat_ms)
{
    for(int i = 0; i < MAX_BUTTONS; i++) {
        if (!s_buttons[i].enabled) {
            s_buttons[i].enabled = true;
            s_buttons[i].gpio = gpio;
            s_buttons[i].state = STATE_INITIAL;
            s_buttons[i].callback = callback;
            s_buttons[i].released_value = released_value;
            s_buttons[i].repeat_ms = repeat_ms;
            gpio_init(gpio);
            gpio_set_dir(gpio, GPIO_IN);
            return true;
        }
    }

    return false;
}

bool easy_buttons_unregister(uint gpio)
{
    for(int i = 0; i < MAX_BUTTONS; i++) {
        if (s_buttons[i].enabled && s_buttons[i].gpio == gpio) {
            s_buttons[i].enabled = false;
            return true;
        }
    }

    return false;
}
