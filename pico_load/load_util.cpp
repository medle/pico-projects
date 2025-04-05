
#include "load.h"
#include <hardware/gpio.h>

void board_led_init()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void board_led_set(bool on)
{
    gpio_put(LED_PIN, on); 
}

