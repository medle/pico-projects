
#include <time.h>
#include <string.h>

#include "LCD_1in14.h"
#include "GUI_Paint.h"
#include "fonts.h"

#include "lcd114.h"

static uint16_t *sImage = NULL;

bool lcdInit()
{
    DEV_Module_Init();
    //DEV_SET_PWM(10); // backlight is left as default
    LCD_1IN14_Init(HORIZONTAL);

    if (sImage == NULL) {
        int imageSize = LCD_1IN14_HEIGHT * LCD_1IN14_WIDTH * 2;
        sImage = (uint16_t *)malloc(imageSize);
        if(sImage == NULL) return false;
    }

    Paint_NewImage((UBYTE *)sImage, LCD_1IN14.WIDTH, LCD_1IN14.HEIGHT, ROTATE_0, BLACK);
    Paint_SetScale(65); // magic number
    Paint_Clear(BLACK);
    lcdUpdateDisplay();

    return true;
}

/// Sends buffer memory image to physical display. 
/// Timing: 70ms on Pi Pico.
void lcdUpdateDisplay()
{
    LCD_1IN14_Display(sImage);
}

void lcdClear(uint16_t color)
{
    lcdFillRect(0, 0, LCD_1IN14_WIDTH, LCD_1IN14_HEIGHT, color);
}

void lcdFillRect(int x1, int y1, int x2, int y2, uint16_t color)
{
    Paint_DrawRectangle(x1, y1, x2, y2, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

void lcdDrawRect(int x1, int y1, int x2, int y2, uint16_t color, int lineWidth)
{
    Paint_DrawRectangle(x1, y1, x2, y2, color, lineWidth, DRAW_FILL_EMPTY);
}

void lcdDrawLine(int x1, int y1, int x2, int y2, uint16_t color, int lineWidth)
{
    Paint_DrawLine(x1, y1, x2, y2, color, lineWidth, LINE_STYLE_SOLID);
}

static sFONT *getFont(LcdFontType fontType)
{
    switch (fontType) {
        case LCD_FONT19: return &Font10x19Fixedsys;
        case LCD_FONT23: return &Font15x23Lucida;
        case LCD_FONT26: return &Font16x26Consolas; 
        default: return NULL;
    }
}

bool lcdDrawString(int x, int y, const char *str, 
    LcdFontType fontType, uint16_t foreColor, uint16_t backColor)
{
    sFONT *pFont = getFont(fontType);
    if (pFont == NULL) return false;

    Paint_DrawString_EN(x, y, str, pFont, backColor, foreColor);
    return true;
}

LcdSize lcdMeasureString(const char *str, LcdFontType fontType)
{
    LcdSize size = { 0, 0 };
    sFONT *pFont = getFont(fontType);
    if (pFont != NULL) { 
        size.height = pFont->Height;
        size.width = strlen(str) * pFont->Width;
    }
    return size;
}

// Returns a 16-bit LCD color value (RED=5bit, GREEN=6bit, BLUE=5bit)
uint16_t lcdMakeColor(uint8_t red, uint8_t green, uint8_t blue)
{
    return (blue * 0x1F / 0xFF) | 
           (((green * 0x3F) / 0xFF) << 5) |
           (((red * 0x1F) / 0xFF) << 11);
}

typedef struct KeyPin {
    LcdKeyType keyType;
    uint8_t gpio;
    bool lastState;
} KeyPin;

// Initial state of pins with pull-ups is high.
static KeyPin keyPins[LCD_NUM_KEYS] = {
    { LCD_KEY_A, 19, true },
    { LCD_KEY_B, 17, true },
    { LCD_KEY_LEFT, 16, true },
    { LCD_KEY_UP, 6, true },
    { LCD_KEY_RIGHT, 22, true },
    { LCD_KEY_DOWN, 18, true },
    { LCD_KEY_CENTER, 7, true }
};

void lcdInitKeys()
{
    // Init all gpios for all keys as input with pull-up.
    for (int i = 0; i < LCD_NUM_KEYS; i++) {
        uint gpio = keyPins[i].gpio;
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
}

#define DEBOUNCE_DURATION_US 10000

static bool debounceGpio(uint gpio)
{
    bool lastState = gpio_get(gpio);
    uint64_t lastTime = time_us_64();
    for(;;) {
        bool state = gpio_get(gpio);
        uint64_t time = time_us_64();

        if (state != lastState || time < lastTime) {
            lastTime = time;
            lastState = state;
        } else {
            if (time - lastTime > DEBOUNCE_DURATION_US) {
                return state;
            }
        }
    }
}

LcdKeyEvent lcdGetKeyEvent()
{
    LcdKeyEvent event;
    event.ready = false;

    for (int i = 0; i < LCD_NUM_KEYS; i++) {
        uint gpio = keyPins[i].gpio;
        bool newState = gpio_get(gpio);
        bool lastState = keyPins[i].lastState; 

        if (newState != lastState) {
            newState = debounceGpio(gpio);
            if (newState != lastState) {
                event.ready = true;
                event.keyType = keyPins[i].keyType;
                event.keyDown = !newState; // pull-down is on when released
                keyPins[i].lastState = newState;
                break;
            }
        }
    }

    return event;
}