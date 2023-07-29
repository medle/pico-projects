
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

    int nFrames = 0;
    WaitToken waitToken;
    while (true) {

        if (running) {
            if(waitCompleted(&waitToken, true)) {
                nFrames += 1;

                uint64_t t0 = time_us_64(); 
                lcdUpdateDisplay();
                uint l = (uint)(time_us_64() - t0);

                char buf[20];
                sprintf(buf, "%d %dus", nFrames, l);
                lcdDrawString(0, 0, buf, LCD_FONT23, LCD_WHITE, LCD_BLACK);
                lcdUpdateDisplay();
            }
        }

        LcdKeyEvent event = lcdGetKeyEvent();
        if (!event.ready) continue;

        if (event.keyType == LCD_KEY_A && event.keyDown) {
            running = !running;      
            if (running) {
                pwmStart(pwmHz, pwmDuty);
                beginWait(&waitToken, 500); 
            }
            else {
                pwmStop();
            }  
            drawStartStopButton(running);
        }
    }    
}


