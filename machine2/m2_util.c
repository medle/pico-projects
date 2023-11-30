
#include "m2_globals.h"

static bool ledInitDone = 0;

void ledSet(bool on)
{
    const uint ledPin = PICO_DEFAULT_LED_PIN;
    if(!ledInitDone) {
        gpio_init(ledPin);
        gpio_set_dir(ledPin, GPIO_OUT);
        ledInitDone = true;
    }

   gpio_put(ledPin, on);
}

void ledRunStartupWelcome()
{
    ledBlink(5, 200);
}

void ledBlink(int numBlinks, int msDelayEach)
{
    if(msDelayEach <= 0) msDelayEach = 100;
  
    bool high = false;
    for(int i = 0; i < numBlinks * 2; i++) {
        ledSet(high = ! high);
        sleep_ms(msDelayEach / 2);
    }  
}

void __expect(int value, int expected, const char *file, int line, const char *expr)
{
    if(value != expected) __assert_failure(value, file, line, expr);
}

void __assert_failure(int value, const char *file, int line, const char *expr)
{
    for(;;) {
        ledBlink(10, 80); // 10 blinks 80ms each
        sleep_ms(200);
        printf("Assert failed: value=%d line=%s file=%s expr=%s\n", value, line, file, expr);
    }
}  