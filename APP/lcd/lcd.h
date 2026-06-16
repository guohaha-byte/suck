#ifndef _lcd_H
#define _lcd_H

#include "system.h"

/*******************************************************************************
 * HX8357DN 3.5" TFT, 16-bit FSMC 8080 parallel interface.
 * PuZhong QiLin F407 board wiring (from schematic):
 *   CS = FSMC_NE4 (PG12)   RS = FSMC_A6 (PF12)
 *   WR = FSMC_NWE (PD5)    RD = FSMC_NOE (PD4)
 *   D0..D15 = PD14 PD15 PD0 PD1 PE7 PE8 PE9 PE10 PE11 PE12 PE13 PE14 PE15 PD8 PD9 PD10
 *   BL (backlight) = PB0 (active high, adjust LCD_BL_* if board differs)
 *
 * FSMC Bank1 sub-bank4 base = 0x6C000000. A6 used as RS selects the data
 * (RAM) cycle, so:
 *   LCD_REG  access -> 0x6C000000          (A6=0, command)
 *   LCD_RAM  access -> 0x6C00007E offset   (A6=1, data)
 ******************************************************************************/

typedef struct
{
    vu16 LCD_REG;   /* command  (RS=0) */
    vu16 LCD_RAM;   /* data     (RS=1) */
} LCD_TypeDef;

#define LCD_BASE        ((u32)(0x6C000000 | 0x0000007E))
#define LCD             ((LCD_TypeDef *)LCD_BASE)

/* backlight control */
#define LCD_BL_RCC      RCC_AHB1Periph_GPIOB
#define LCD_BL_PORT     GPIOB
#define LCD_BL_PIN      GPIO_Pin_0
#define LCD_BL_ON()     GPIO_SetBits(LCD_BL_PORT, LCD_BL_PIN)
#define LCD_BL_OFF()    GPIO_ResetBits(LCD_BL_PORT, LCD_BL_PIN)

/* LCD important info, filled by LCD_Init() (landscape 480x320) */
typedef struct
{
    u16 width;
    u16 height;
    u16 id;
    u8  dir;        /* 0 = portrait, 1 = landscape */
} _lcd_dev;
extern _lcd_dev lcddev;

/* Common 16-bit (RGB565) colors */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define YELLOW      0xFFE0
#define GRAY        0x8430
#define DARKGRAY    0x4208
#define ORANGE      0xFC00
#define LIGHTBLUE   0x7D7C

/* foreground / background pen colors */
extern u16 POINT_COLOR;
extern u16 BACK_COLOR;

void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(u16 color);
void LCD_SetCursor(u16 x, u16 y);
void LCD_DrawPoint(u16 x, u16 y);
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color);
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_ShowChar(u16 x, u16 y, u8 chr, u8 size, u8 mode);
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size);
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, char *p);

#endif
