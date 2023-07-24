
#include <stdio.h>
#include "pico/stdlib.h"

#include <lcd114.h>

int main() {
    stdio_init_all();
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    sleep_ms(500);

    lcdInit();
    lcdInitKeys();

    lcdDrawLine(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, lcdMakeColor(0, 50, 0), 2);
    lcdFillRect(0, 70, 50, 134, lcdMakeColor(0, 0, 50));
    lcdDrawText(0, 0, __TIME__, LCD_FONT23, LCD_GRAY, LCD_BLACK);     
    lcdUpdateDisplay();

    printf("#starting...");

    uint i = 0; 
    while (true) {

        LcdKeyEvent event;
        if (lcdGetKeyEvent(&event)) {
            if (event.keyDown) {
                bool on = event.keyType == LCD_KEY_A;
                gpio_put(LED_PIN, on);
            }

            char buf[20];
            sprintf(buf, "i=%d d=%d k=%d", ++i, event.keyDown, event.keyType);
            lcdDrawText(0, 24, buf, LCD_FONT23, LCD_YELLOW, LCD_BLACK);
            lcdUpdateDisplay();
        } 
        

        //gpio_put(LED_PIN, 1);
        //sleep_ms(500);
        //gpio_put(LED_PIN, 0);
        //sleep_ms(250);
    }
}
