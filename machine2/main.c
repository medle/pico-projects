
#include "machine.h"
#include "lcd114.h"

int main()
{
    stdio_init_all();
    lcdInit();
    lcdInitKeys();

    showLogo(); 

    bool running = false;
    drawStartStopButton(!running);

    while(true) {
        LcdKeyEvent event = lcdGetKeyEvent();
        if (!event.ready) continue;

        if (event.keyType == LCD_KEY_A && event.keyDown) {
            running = !running;        
            drawStartStopButton(!running);
        }         

    }    

}

