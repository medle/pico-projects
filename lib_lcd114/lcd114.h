
#ifndef __LCD114_H
#define __LCD114_H

#include <stdbool.h>

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

#define LCD_WHITE          0xFFFF
#define LCD_BLACK          0x0000
#define LCD_BLUE           0x001F
#define LCD_RED            0xF800
#define LCD_MAGENTA        0xF81F
#define LCD_GREEN          0x07E0
#define LCD_CYAN           0x7FFF
#define LCD_YELLOW         0xFFE0
#define LCD_BROWN          0XBC40
#define LCD_GRAY           0X8430

typedef enum {
    LCD_FONT19 = 0,
    LCD_FONT23,
    LCD_FONT26   
} LcdFontType;

typedef struct LcdSize {
    uint16_t width;
    uint16_t height;
} LcdSize;

bool lcdInit();
void lcdUpdateDisplay();
void lcdClear(uint16_t color);
void lcdDrawRect(int x1, int y1, int x2, int y2, uint16_t color, int lineWidth);
void lcdFillRect(int x1, int y1, int x2, int y2, uint16_t color);
void lcdDrawLine(int x1, int y1, int x2, int y2, uint16_t color, int lineWidth);
bool lcdDrawString(int x, int y, const char *str, 
    LcdFontType fontType, uint16_t foreColor, uint16_t backColor);
LcdSize lcdMeasureString(const char *str, LcdFontType fontType);

uint16_t lcdMakeColor(uint8_t red, uint8_t green, uint8_t blue);

typedef enum {
    LCD_KEY_A,
    LCD_KEY_B,
    LCD_KEY_LEFT,
    LCD_KEY_UP,
    LCD_KEY_RIGHT,
    LCD_KEY_DOWN,
    LCD_KEY_CENTER,
    LCD_NUM_KEYS
} LcdKeyType;

typedef struct LcdKeyEvent {
    LcdKeyType keyType;
    bool keyDown;
    bool ready;
} LcdKeyEvent;

void lcdInitKeys();
LcdKeyEvent lcdGetKeyEvent();

#endif