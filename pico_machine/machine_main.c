
#include "machine.h"

static void configure_pwm();

int main() 
{
    stdio_init_all(); 
    led_run_startup_welcome();

    sleep_ms(1000);

    printf("starting\n"); 

    mach_start_pwm(8300, 25);

    while (true) {
        sleep_ms(1000);
        led_set(1);
        sleep_ms(5);
        led_set(0);
    }
}

