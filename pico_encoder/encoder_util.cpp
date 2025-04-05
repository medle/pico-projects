
#include "encoder.h"
#include <hardware/gpio.h>

void encoder_led_init()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void encoder_led_set(bool on)
{
    gpio_put(LED_PIN, on); 
}

