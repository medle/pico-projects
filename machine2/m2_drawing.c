
#include "m2_global.h"
#include "lcd114.h"
#include <math.h>

#define MENU_HEIGHT (23 + 2)

static void drawLogoString(int y, const char *str, LcdFontType fontType)
{
    LcdSize size = lcdMeasureString(str, fontType);
    int x = (LCD_WIDTH - size.width) / 2;
    lcdDrawString(x, y, str, fontType, LCD_GRAY, LCD_BLACK);
}

void drawLogo()
{
    int y = (LCD_HEIGHT - 23 * 4) / 2;
    drawLogoString(y, "Machine2", LCD_FONT23);
    drawLogoString(y + 23 * 1, "built", LCD_FONT23);     
    drawLogoString(y + 23 * 2, __DATE__, LCD_FONT23);     
    drawLogoString(y + 23 * 3, __TIME__, LCD_FONT23);     
}

void drawStartStopButton(bool running, bool tick)
{
    char *title = running ? "Run" : "Off";
    LcdSize size = lcdMeasureString(title, LCD_FONT23);
    int x = LCD_WIDTH - size.width;
    int foreColor = running ? (tick ? LCD_WHITE : lcdMakeColor(200, 200, 200)) : LCD_BLACK;
    int backColor = running ? (tick ? LCD_RED : lcdMakeColor(200, 0, 0)) : LCD_GRAY;
    lcdDrawString(x, 0, title, LCD_FONT23, foreColor, backColor);
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

    // display y coordinate grows from top of the display to the bottom,
    // y is inverted so we invert the graph values
    float x1 = GRAPH_X;
    const int yBottom = GRAPH_Y + GRAPH_HEIGHT - 1;
    float y1 = yBottom - (values[0] * yScaler);

    for (int i = 1; i < numValues; i++) {
        float x2 = x1 + xStep;
        float vScaled = values[i] * yScaler;
        if (vScaled >= GRAPH_HEIGHT) vScaled = GRAPH_HEIGHT - 1; 
        float y2 = yBottom - vScaled;
        lcdDrawLine((int)x1, (int)y1, (int)x2, (int)y2, color, 1);
        x1 = x2;
        y1 = y2;         
    }  
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
