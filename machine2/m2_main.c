
#include "m2_global.h"
#include "lcd114.h"

static uint pwmHz = 5600;
static float pwmDuty = 0.25;

int main()
{
    stdio_init_all();
    lcdInit();
    lcdInitKeys();
    pwmInit();

    showLogo(); 

    uint8_t data[100];
    generateSineWaveValues(data, sizeof(data));
    drawGraphGrid(); 
    drawGraph(data, sizeof(data), LCD_RED);

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


