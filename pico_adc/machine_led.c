
#include "machine.h"

static bool ledInitDone = 0;

void ledSet(bool on)
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    if(!ledInitDone) {
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        ledInitDone = true;
    }

   gpio_put(LED_PIN, on);
}

void ledRunStartupWelcome()
{
    ledBlink(5, 200);
}

void ledBlink(int num_blinks, int ms_delay_each)
{
    if(ms_delay_each <= 0) ms_delay_each = 100;
  
    bool high = false;
    for(int i = 0; i < num_blinks * 2; i++) {
        ledSet(high = ! high);
        sleep_ms(ms_delay_each / 2);
    }  
}

void __expect0(int code, const char *file, int line, const char *expr)
{
    if(code != 0) __assert_failure(code, file, line, expr);
}

void __assert_failure(int code, const char *file, int line, const char *expr)
{
    for(;;) {
        ledBlink(10, 80); // 10 blinks 80ms each
        sleep_ms(200);
        printf("Assert failed: code=%d line=%s file=%s expr=%s\n", code, line, file, expr);
    }
}  
