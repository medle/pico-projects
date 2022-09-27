
#include "machine.h"

int main() 
{
    stdio_init_all(); 
    led_run_startup_welcome();
    mach_init();

    while (true) {
        int ch = getchar_timeout_us(1000000);
        if (ch == PICO_ERROR_TIMEOUT) continue;
        command_parse_input_char((char)ch); 
    }
}

