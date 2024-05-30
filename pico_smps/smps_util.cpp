
#include "smps.h"
#include "easy_pwm.h"

void smps_led_init()
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void smps_led_set(bool on)
{
    gpio_put(LED_PIN, 1); 
}

void smps_sound_enable(bool enable)
{
    if (enable) easy_pwm_enable(SOUND_GEN_PIN, SOUND_GEN_HZ, 0.5);  
    else easy_pwm_disable(SOUND_GEN_PIN);
}

void smps_sound_play_beeps(int n)
{
    for(int i = 0; i < n; i++) {
      smps_sound_enable(true);   
      sleep_ms(100);
      smps_sound_enable(false);
      sleep_ms(100);
    }
}
