
#include "pico/stdlib.h"

static bool led_init_done = 0;
static void set_led(bool on);
static void run_startup_led_flash();

int main() 
{
    while (true) {
        set_led(1);
        sleep_ms(500);
        set_led(0);
        sleep_ms(250);
    }
}

static void run_startup_led_flash()
{
    for(int i = 0; i < 2; i++) {
        set_led(1);
        sleep_ms(250);
        set_led(0);
        sleep_ms(250);
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