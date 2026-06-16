#include "gui.h"
#include "lcd.h"

/*******************************************************************************
 * Simple graphical dashboard for the light-monitor demo.
 * Layout (480 x 320 landscape):
 *   +--------------------------------------------------+
 *   |              Light Monitor System                |  title bar
 *   +--------------------------------------------------+
 *   |  Light :  78 %                                   |
 *   |  LED1  :  ON                                     |
 *   |  LED2  :  OFF                                    |
 *   |  WiFi  :  Connected                              |
 *   |  MQTT  :  Connected                              |
 *   +--------------------------------------------------+
 ******************************************************************************/

#define LBL_X        20      /* label column            */
#define VAL_X        170     /* value column            */
#define ROW0_Y       90      /* first data row          */
#define ROW_H        40      /* row pitch               */
#define FSIZE        24      /* data font size          */

/* cached previous values so we only repaint on change; 0xFF = "force draw" */
static u8 last_light = 0xFF;
static u8 last_led1  = 0xFF;
static u8 last_led2  = 0xFF;
static u8 last_wifi  = 0xFF;
static u8 last_mqtt  = 0xFF;

static u16 row_y(u8 i) { return ROW0_Y + i * ROW_H; }

void UI_Init(void)
{
    /* clear background */
    BACK_COLOR  = WHITE;
    LCD_Clear(WHITE);

    /* title bar */
    LCD_Fill(0, 0, lcddev.width - 1, 50, BLUE);
    POINT_COLOR = WHITE;
    BACK_COLOR  = BLUE;
    LCD_ShowString(90, 14, 400, 24, FSIZE, "Light Monitor System");

    /* static labels */
    POINT_COLOR = BLACK;
    BACK_COLOR  = WHITE;
    LCD_ShowString(LBL_X, row_y(0), 160, 24, FSIZE, "Light :");
    LCD_ShowString(LBL_X, row_y(1), 160, 24, FSIZE, "LED1  :");
    LCD_ShowString(LBL_X, row_y(2), 160, 24, FSIZE, "LED2  :");
    LCD_ShowString(LBL_X, row_y(3), 160, 24, FSIZE, "WiFi  :");
    LCD_ShowString(LBL_X, row_y(4), 160, 24, FSIZE, "MQTT  :");

    /* force first full refresh */
    last_light = last_led1 = last_led2 = last_wifi = last_mqtt = 0xFF;
}

/* draw an ON/OFF state value with color */
static void draw_state(u8 row, u8 on, const char *son, const char *soff)
{
    BACK_COLOR  = WHITE;
    /* clear value area first */
    LCD_Fill(VAL_X, row_y(row), VAL_X + 200, row_y(row) + FSIZE - 1, WHITE);
    POINT_COLOR = on ? GREEN : GRAY;
    LCD_ShowString(VAL_X, row_y(row), 200, 24, FSIZE, (char *)(on ? son : soff));
}

void UI_Update(u8 light, u8 led1, u8 led2, u8 wifi, u8 mqtt)
{
    if (light != last_light)
    {
        BACK_COLOR  = WHITE;
        POINT_COLOR = BLACK;
        LCD_Fill(VAL_X, row_y(0), VAL_X + 120, row_y(0) + FSIZE - 1, WHITE);
        LCD_ShowNum(VAL_X, row_y(0), light, 3, FSIZE);
        LCD_ShowString(VAL_X + 3 * 12 + 6, row_y(0), 40, 24, FSIZE, "%");
        last_light = light;
    }
    if (led1 != last_led1) { draw_state(1, led1, "ON", "OFF"); last_led1 = led1; }
    if (led2 != last_led2) { draw_state(2, led2, "ON", "OFF"); last_led2 = led2; }
    if (wifi != last_wifi) { draw_state(3, wifi, "Connected", "Disconnected"); last_wifi = wifi; }
    if (mqtt != last_mqtt) { draw_state(4, mqtt, "Connected", "Disconnected"); last_mqtt = mqtt; }

    POINT_COLOR = BLACK;
    BACK_COLOR  = WHITE;
}
