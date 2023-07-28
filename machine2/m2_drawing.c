
#include "m2_global.h"
#include "lcd114.h"
#include <math.h>

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

// Where graph is located on LCD display.
#define GRAPH_X 0
#define GRAPH_Y MENU_HEIGHT
#define GRAPH_HEIGHT (LCD_HEIGHT - GRAPH_Y)
#define GRAPH_WIDTH LCD_WIDTH

void drawGraphGrid()
{
    int axisColor = lcdMakeColor(50, 50, 50);
    int backColor = lcdMakeColor(0, 0, 50);

    int graphRight = GRAPH_X + GRAPH_WIDTH;
    int graphBottom = GRAPH_Y + GRAPH_HEIGHT;

    lcdFillRect(GRAPH_X, GRAPH_Y, graphRight, graphBottom, backColor);

    const int yBands = 10;
    float yStep = (float)GRAPH_HEIGHT / yBands;  
    for (int i = 0; i <= yBands; i++) {
        int y = GRAPH_Y + (int)(i * yStep);
        int lineWidth = (i == yBands / 2) ? 3 : 1;
        lcdDrawLine(GRAPH_X, y, graphRight, y, axisColor, lineWidth);    
    }

    const int xBands = 4;
    float xStep = (float)GRAPH_WIDTH / xBands;
    for (int i = 0; i <= xBands; i++) {
        int x = GRAPH_X + (int)(i * xStep);
        lcdDrawLine(x, GRAPH_Y, x, graphBottom, axisColor, 1); 
    }
}

void drawGraph(uint8_t *values, int numValues, int color)
{
    if (values == NULL || numValues <= 0) return;

    float xStep = (float)GRAPH_WIDTH / numValues;
    float yScaler = (float)GRAPH_HEIGHT / 256; 

    float x1 = GRAPH_X;
    float y1 = GRAPH_Y + values[0] * yScaler;

    for (int i = 1; i < numValues; i++) {
        float x2 = x1 + xStep;
        float y2 = GRAPH_Y + values[i] * yScaler;
        lcdDrawLine((int)x1, (int)y1, (int)x2, (int)y2, color, 1);
        x1 = x2;
        y1 = y2;         
    }  
}

