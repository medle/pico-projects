
#include "m2_globals.h"
#include "lcd114.h"

#define CONFIG_MAGIC 0xcafe
#define CONFIG_EEPROM_ADDRESS 0x00
typedef struct ConfigType { uint16_t magic; uint16_t pwmHz; float pwmDuty; } ConfigType;
#define CONFIG_SIZE sizeof(ConfigType)

typedef enum ScaleType { 
    SCALE_NONE, 
    SCALE_ACS712_5A, 
    SCALE_ACS712_30A,
    SCALE_LEMCAS6_20A, 
    SCALE_LEMCAS50 
} ScaleType;

// steel bolt
static ConfigType _defaultConfig = { .magic = CONFIG_MAGIC, .pwmHz = 3020, .pwmDuty = 2.5 };
static ConfigType _config;

static bool _isRunning = false;
static int _nFrames = 0;

static void saveConfig();
static void restoreConfig();
static void showSystemBootDisplay();
static void refreshStartStopButton();
static void setRunningState(bool on);
static void refreshDisplayContent();
static LcdKeyEvent waitKeyEvent(int timeoutMillis);
static void handleKeyDown(LcdKeyType keyType);

static void scaleCapturedSamples(uint8_t *samples, uint numSamples, ScaleType scaleType);
static uint8_t findMaxSample(uint8_t *samples, uint numSamples);
static uint8_t findAverageSample(uint8_t *samples, uint numSamples);
static float computeRms(uint8_t *samples, uint numSamples);

int main()
{
    stdio_init_all();
    lcdInit();
    lcdInitKeys();
    machPwmInit();
    machAdcInit();
    showSystemBootDisplay();
    eepromInit();
    restoreConfig();
    printf("M2 Ready.\n");

    for (;;) {

        LcdKeyEvent event = waitKeyEvent(500);
        if (event.ready && event.keyDown) handleKeyDown(event.keyType); 

        // timeout or key, refresh display
        if (_isRunning) {
            refreshDisplayContent();
        }
    }    
}

static void handleKeyDown(LcdKeyType keyType)
{
    switch (keyType) {

        case LCD_KEY_A: 
            setRunningState(!_isRunning);
            refreshStartStopButton();
            break;

        case LCD_KEY_UP:
            if (_isRunning) {
                _config.pwmHz += 20;
                machPwmChangeWaveform(_config.pwmHz, _config.pwmDuty);
                saveConfig();
            }
            break;

        case LCD_KEY_DOWN:
            if (_isRunning) {
                _config.pwmHz -= 20;
                machPwmChangeWaveform(_config.pwmHz, _config.pwmDuty);
                saveConfig();
            } 
            break;   

        case LCD_KEY_LEFT:
            if (_isRunning) {
                _config.pwmDuty -= 0.1;
                machPwmChangeWaveform(_config.pwmHz, _config.pwmDuty);
                saveConfig();
            }
            break;    

        case LCD_KEY_RIGHT:
            if (_isRunning) {
                _config.pwmDuty += 0.1;
                machPwmChangeWaveform(_config.pwmHz, _config.pwmDuty);
                saveConfig();
            }
            break;    
    }
}

static void showSystemBootDisplay()
{
    drawLogo();
    lcdUpdateDisplay();
    refreshStartStopButton(); 
}

static void refreshStartStopButton()
{
    drawStartStopButton(_isRunning, false);
    lcdUpdateDisplay(); 
}

static void setRunningState(bool on)
{
   _isRunning = on;
    if (_isRunning) {
        machPwmStart(_config.pwmHz, _config.pwmDuty, machAdcHandlePeriodEnd);
        //machSenseEnable(true);
    } else {
        //machSenseEnable(false);
        machPwmStop();
    }
}

#define DATALEN 512
static uint8_t data[DATALEN];
extern uint _selectedCompareValue;
extern PwmTopDivider _selectedTopDivider;

static void refreshDisplayContent()
{
    _nFrames += 1;
    drawStartStopButton(_isRunning, _nFrames & 1);

    drawGraphGrid();

    char buf[50];
    uint numSamples = 0;

    machAdcInit(true);
    numSamples = machAdcMeasurePeriod(ADC_CH1, data, DATALEN);
    float rms = computeRms(data, numSamples);
    float rmsA = rms * 18;  
    sprintf(buf, "num=%d rms=%.2fA", numSamples, rmsA);
    LcdSize size = lcdMeasureString(buf, LCD_FONT19);
    lcdDrawString(0, 19, buf, LCD_FONT19, LCD_WHITE, LCD_BLACK);

    sprintf(buf, "hz=%d duty=%.2f", _config.pwmHz, _config.pwmDuty); 
    lcdDrawString(0, LCD_HEIGHT-19, buf, LCD_FONT19, LCD_WHITE, LCD_BLACK);

    scaleCapturedSamples(data, numSamples, SCALE_LEMCAS6_20A);
    drawGraph(data, numSamples, LCD_GREEN);

    lcdUpdateDisplay();
}

static float computeRms(uint8_t *samples, uint numSamples)
{
    int square = 0;
    for(int i=0; i<numSamples; i++) {
        int offset = (int)samples[i] - 127;
        square += offset;
    }

    int fullSquare = numSamples * 127;
    return (float)square / fullSquare;
}

static uint8_t findAverageSample(uint8_t *samples, uint numSamples)
{
    if (numSamples == 0) return 0;
    int sum = 0;
    for (int i = 0; i<numSamples; i++) {
        sum += samples[i];
    }     
    return sum / numSamples; 
}

static uint8_t findMaxSample(uint8_t *samples, uint numSamples)
{
    if (numSamples == 0) return 0;
    uint8_t max = samples[0];
    for (int i = 1; i < numSamples; i++) {
        if (samples[i] > max) max = samples[i];
    }     
    return max; 
}

static void scaleCapturedSamples(
    uint8_t *samples, uint numSamples, ScaleType scaleType)
{
    float scale = 1; 
    switch (scaleType) {
        // 0->5A: 2.5->3.4V in 5V mode 
        // in bytes: 127->255*3.4/5=173
        // f(127)=127, f(173)=255, f(x)=127+(x-127)*2.76 
        case SCALE_ACS712_5A: scale = 2.76; break;
        // 0->30A: 2.5->4.4V in 5V mode
        // in bytes: 127->255*4.4/5=224
        // f(127)=127, f(224)=255, f(x)=127+(x-127)*1.31
        case SCALE_ACS712_30A: scale = 1.31; break;

        // 2.5V=>0A, 4.625V=>20A
        case SCALE_LEMCAS6_20A: scale = 2.5 / (4.625 - 2.5);
    }

    for (int i = 0; i < numSamples; i++) {
        samples[i] = (uint8_t)(127 + (samples[i] - 127) * scale);
    }     
}

static LcdKeyEvent waitKeyEvent(int timeoutMillis)
{
    LcdKeyEvent event;
    event.ready = false;
    uint64_t startTime = time_us_64();
    for (;;) {
        uint64_t elapsedMillis = (time_us_64() - startTime) / 1000;
        if (elapsedMillis > timeoutMillis) break;
        event = lcdGetKeyEvent();
        if (event.ready) break; 
    }

    return event;
}

static void saveConfig()
{
    int ret = eepromWriteBytes(CONFIG_EEPROM_ADDRESS, (uint8_t *)&_config, CONFIG_SIZE);
    expect(CONFIG_SIZE, ret);
}

static void restoreConfig()
{
    int ret = eepromReadBytes(CONFIG_EEPROM_ADDRESS, (uint8_t *)&_config, CONFIG_SIZE);
    if (ret != CONFIG_SIZE || _config.magic != CONFIG_MAGIC) {
        _config = _defaultConfig;
    }
}
