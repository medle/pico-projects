
#include "m2_global.h"
#include "lcd114.h"

static uint _pwmHz = 5600;
static float _pwmDuty = 0.15;
static bool _isRunning = false;
static WaitToken _refreshWaitToken;
static uint _refreshWaitMs = 430;
static int _nFrames = 0;

static void showSystemBootedDisplay();
static void setRunningState(bool on);
static void refreshDisplayContent();

int main()
{
    stdio_init_all();
    lcdInit();
    lcdInitKeys();
    machPwmInit();
    machAdcInit();
    showSystemBootedDisplay();

    while (true) {

        LcdKeyEvent event = lcdGetKeyEvent();
        if (event.ready) {
            if (event.keyType == LCD_KEY_A && event.keyDown) {
                setRunningState(!_isRunning);
            }
        }

        if (_isRunning) {
            if (waitCompleted(&_refreshWaitToken, true)) {
                refreshDisplayContent();
            }
        }
    }    
}

static void showSystemBootedDisplay()
{
    drawLogo();
    drawStartStopButton(_isRunning, false);
    lcdUpdateDisplay(); 
}

static void setRunningState(bool on)
{
   _isRunning = on;
    if (_isRunning) {
        machPwmStart(_pwmHz, _pwmDuty, machAdcHandlePeriodEnd);
        beginWait(&_refreshWaitToken, _refreshWaitMs); 
    } else {
        machPwmStop();
    }

   drawStartStopButton(_isRunning, false);
   lcdUpdateDisplay();
}

static uint8_t data[256];

static void refreshDisplayContent()
{
    _nFrames += 1;
    drawStartStopButton(_isRunning, _nFrames & 1);

    uint numSamples = machAdcMeasurePeriod(ADC_CH0, data, sizeof(data));  
    drawGraphGrid(); 
    drawGraph(data, numSamples, LCD_RED);

    char buf[50];
    sprintf(buf, "%d", numSamples);
    lcdDrawString(0, 0, buf, LCD_FONT23, LCD_WHITE, LCD_BLACK);
 
    lcdUpdateDisplay();
}
