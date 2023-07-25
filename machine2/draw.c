
#include "machine.h"
#include "lcd114.h"

void drawStartStopButton(bool on)
{
    char *title = on ? "Run" : "Off";
    LcdSize size = lcdMeasureString(title, LCD_FONT23);
    int x = LCD_WIDTH - size.width;
    int color = on ? LCD_GRAY : LCD_RED;
    lcdDrawString(x, 0, title, LCD_FONT23, color, LCD_BLACK);
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
