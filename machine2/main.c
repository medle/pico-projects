
#include "machine.h"
#include "lcd114.h"

static uint pwmHz = 5600;
static float pwmDuty = 0.25;

int main()
{
    stdio_init_all();
    lcdInit();
    lcdInitKeys();
    pwmInit(PWM_GPIO);

    showLogo(); 

    bool running = false;
    drawStartStopButton(running);

    while(true) {
        LcdKeyEvent event = lcdGetKeyEvent();
        if (!event.ready) continue;

        if (event.keyType == LCD_KEY_A && event.keyDown) {
            running = !running;      
            if (running) pwmStart(pwmHz, pwmDuty);
            else pwmStop();  
            drawStartStopButton(running);
        }         

    }    
}


