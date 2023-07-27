
#include <math.h>
#include "machine.h"
#include "lcd114.h"

#define MENU_HEIGHT 23

void drawStartStopButton(bool on)
{
    char *title = on ? "Run" : "Off";
    LcdSize size = lcdMeasureString(title, LCD_FONT23);
    int x = LCD_WIDTH - size.width;
    int foreColor = on ? LCD_YELLOW : LCD_BLACK;
    int backColor = on ? lcdMakeColor(0, 60, 0) : LCD_GRAY;
    lcdDrawString(x, 0, title, LCD_FONT23, foreColor, backColor);
    lcdUpdateDisplay();
}

static void drawLogoString(int y, const char *str, LcdFontType fontType)
{
    LcdSize size = lcdMeasureString(str, fontType);
    int x = (LCD_WIDTH - size.width) / 2;
    lcdDrawString(x, y, str, fontType, LCD_GRAY, LCD_BLACK);
}

void showLogo()
{
    int y = (LCD_HEIGHT - 23 * 4) / 2;
    drawLogoString(y, "Machine2", LCD_FONT23);
    drawLogoString(y + 23 * 1, "built", LCD_FONT23);     
    drawLogoString(y + 23 * 2, __DATE__, LCD_FONT23);     
    drawLogoString(y + 23 * 3, __TIME__, LCD_FONT23);     
    lcdUpdateDisplay();
    sleep_ms(1000);
}

void generateSineWaveValues(uint8_t *values, int numValues)
{
    if (values == NULL || numValues <= 0) return;

    int halfValue = 127;
    float step = 2 * M_PI / numValues;

    for (int i = 0; i < numValues; i++) {
        float x = step * i;
        float y = halfValue + (sin(x) * halfValue);
        values[i] = (uint8_t)y;
    }
}

#define GRAPH_LEFT 0
#define GRAPH_TOP MENU_HEIGHT
#define GRAPH_HEIGHT (LCD_HEIGHT - GRAPH_TOP)
#define GRAPH_WIDTH (LCD_WIDTH)

void drawGraphGrid()
{
    int axisColor = lcdMakeColor(50, 50, 50);
    int backColor = lcdMakeColor(0, 0, 50);

    int right = GRAPH_LEFT + GRAPH_WIDTH;
    int bottom = GRAPH_TOP + GRAPH_HEIGHT;

    lcdFillRect(GRAPH_LEFT, GRAPH_TOP, right, bottom, backColor);
    lcdDrawRect(GRAPH_LEFT, GRAPH_TOP, right, bottom, axisColor, 1);
    int yCenter = GRAPH_TOP + GRAPH_HEIGHT / 2;
    lcdDrawLine(GRAPH_LEFT, yCenter, right, yCenter, axisColor, 1);
    int xCenter = GRAPH_LEFT + GRAPH_WIDTH / 2;
    lcdDrawLine(xCenter, GRAPH_TOP, xCenter, bottom, axisColor, 1);
}

void drawGraph(uint8_t *values, int numValues, int color)
{
    if (values == NULL || numValues <= 0) return;

    float xStep = (float)GRAPH_WIDTH / numValues;
    float yScaler = (float)GRAPH_HEIGHT / 256; 

    float x1 = GRAPH_LEFT;
    float y1 = GRAPH_TOP + values[0] * yScaler;

    for (int i = 1; i < numValues; i++) {
        float x2 = x1 + xStep;
        float y2 = GRAPH_TOP + values[i] * yScaler;
        lcdDrawLine((int)x1, (int)y1, (int)x2, (int)y2, color, 1);
        x1 = x2;
        y1 = y2;         
    }  
}

