#include "lcd.h"
#include "font.h"
#include "SysTick.h"

_lcd_dev lcddev;
u16 POINT_COLOR = BLACK;
u16 BACK_COLOR  = WHITE;

/* ---- low level register / data access over FSMC ---- */
static void LCD_WR_REG(u16 reg)   { LCD->LCD_REG = reg; }
static void LCD_WR_DATA(u16 data) { LCD->LCD_RAM = data; }

/* ---- FSMC + GPIO hardware init ---- */
static void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef       GPIO_InitStructure;
    FSMC_NORSRAMInitTypeDef FSMC_NSInit;
    FSMC_NORSRAMTimingInitTypeDef readTiming;
    FSMC_NORSRAMTimingInitTypeDef writeTiming;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE |
                           RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG |
                           LCD_BL_RCC, ENABLE);
    RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);

    /* backlight pin */
    GPIO_InitStructure.GPIO_Pin   = LCD_BL_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(LCD_BL_PORT, &GPIO_InitStructure);

    /* common AF setup for all FSMC pins */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;

    /* PD0,PD1,PD4,PD5,PD8,PD9,PD10,PD14,PD15 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 |
                                  GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 |
                                  GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /* PE7..PE15 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 |
                                  GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 |
                                  GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* PF12 = FSMC_A6 (RS) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOF, &GPIO_InitStructure);

    /* PG12 = FSMC_NE4 (CS) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* alternate function mapping (AF12 = FSMC) */
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource0,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource4,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource5,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource7,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource8,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9,  GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FSMC);
    GPIO_PinAFConfig(GPIOG, GPIO_PinSource12, GPIO_AF_FSMC);

    /* read timing */
    readTiming.FSMC_AddressSetupTime      = 15;
    readTiming.FSMC_AddressHoldTime       = 0;
    readTiming.FSMC_DataSetupTime         = 60;
    readTiming.FSMC_BusTurnAroundDuration = 0;
    readTiming.FSMC_CLKDivision           = 0;
    readTiming.FSMC_DataLatency           = 0;
    readTiming.FSMC_AccessMode            = FSMC_AccessMode_A;

    /* write timing (faster) */
    writeTiming.FSMC_AddressSetupTime      = 9;
    writeTiming.FSMC_AddressHoldTime       = 0;
    writeTiming.FSMC_DataSetupTime         = 8;
    writeTiming.FSMC_BusTurnAroundDuration = 0;
    writeTiming.FSMC_CLKDivision           = 0;
    writeTiming.FSMC_DataLatency           = 0;
    writeTiming.FSMC_AccessMode            = FSMC_AccessMode_A;

    FSMC_NSInit.FSMC_Bank                = FSMC_Bank1_NORSRAM4;
    FSMC_NSInit.FSMC_DataAddressMux      = FSMC_DataAddressMux_Disable;
    FSMC_NSInit.FSMC_MemoryType          = FSMC_MemoryType_SRAM;
    FSMC_NSInit.FSMC_MemoryDataWidth     = FSMC_MemoryDataWidth_16b;
    FSMC_NSInit.FSMC_BurstAccessMode     = FSMC_BurstAccessMode_Disable;
    FSMC_NSInit.FSMC_WaitSignalPolarity  = FSMC_WaitSignalPolarity_Low;
    FSMC_NSInit.FSMC_AsynchronousWait    = FSMC_AsynchronousWait_Disable;
    FSMC_NSInit.FSMC_WrapMode            = FSMC_WrapMode_Disable;
    FSMC_NSInit.FSMC_WaitSignalActive    = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NSInit.FSMC_WriteOperation      = FSMC_WriteOperation_Enable;
    FSMC_NSInit.FSMC_WaitSignal          = FSMC_WaitSignal_Disable;
    FSMC_NSInit.FSMC_ExtendedMode        = FSMC_ExtendedMode_Enable;
    FSMC_NSInit.FSMC_WriteBurst          = FSMC_WriteBurst_Disable;
    FSMC_NSInit.FSMC_ReadWriteTimingStruct  = &readTiming;
    FSMC_NSInit.FSMC_WriteTimingStruct      = &writeTiming;
    FSMC_NORSRAMInit(&FSMC_NSInit);

    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

/* ---- set the GRAM address window ---- */
static void LCD_SetWindow(u16 sx, u16 sy, u16 ex, u16 ey)
{
    LCD_WR_REG(0x2A);                       /* column address set */
    LCD_WR_DATA(sx >> 8); LCD_WR_DATA(sx & 0xFF);
    LCD_WR_DATA(ex >> 8); LCD_WR_DATA(ex & 0xFF);
    LCD_WR_REG(0x2B);                       /* page address set */
    LCD_WR_DATA(sy >> 8); LCD_WR_DATA(sy & 0xFF);
    LCD_WR_DATA(ey >> 8); LCD_WR_DATA(ey & 0xFF);
    LCD_WR_REG(0x2C);                       /* memory write */
}

void LCD_SetCursor(u16 x, u16 y)
{
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0xFF);
    LCD_WR_DATA(x >> 8); LCD_WR_DATA(x & 0xFF);
    LCD_WR_REG(0x2B);
    LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0xFF);
    LCD_WR_DATA(y >> 8); LCD_WR_DATA(y & 0xFF);
    LCD_WR_REG(0x2C);
}

void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color)
{
    LCD_SetCursor(x, y);
    LCD_WR_DATA(color);
}

void LCD_DrawPoint(u16 x, u16 y)
{
    LCD_Fast_DrawPoint(x, y, POINT_COLOR);
}

void LCD_DisplayOn(void)  { LCD_WR_REG(0x29); LCD_BL_ON(); }
void LCD_DisplayOff(void) { LCD_WR_REG(0x28); LCD_BL_OFF(); }

/* fill the whole screen */
void LCD_Clear(u16 color)
{
    u32 i, total = (u32)lcddev.width * lcddev.height;
    LCD_SetWindow(0, 0, lcddev.width - 1, lcddev.height - 1);
    for (i = 0; i < total; i++)
        LCD->LCD_RAM = color;
}

/* fill a rectangle with one color */
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
    u32 i, n;
    LCD_SetWindow(sx, sy, ex, ey);
    n = (u32)(ex - sx + 1) * (ey - sy + 1);
    for (i = 0; i < n; i++)
        LCD->LCD_RAM = color;
}

/* Bresenham line */
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
    int dx = x2 - x1, dy = y2 - y1;
    int ux = dx > 0 ? 1 : -1, uy = dy > 0 ? 1 : -1;
    int x = x1, y = y1, eps;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > dy)
    {
        eps = dx >> 1;
        while (x != (int)x2) { LCD_DrawPoint(x, y); eps -= dy; if (eps < 0){ y += uy; eps += dx; } x += ux; }
    }
    else
    {
        eps = dy >> 1;
        while (y != (int)y2) { LCD_DrawPoint(x, y); eps -= dx; if (eps < 0){ x += ux; eps += dy; } y += uy; }
    }
    LCD_DrawPoint(x2, y2);
}

void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_DrawLine(x1, y1, x2, y1);
    LCD_DrawLine(x1, y1, x1, y2);
    LCD_DrawLine(x1, y2, x2, y2);
    LCD_DrawLine(x2, y1, x2, y2);
}

/* power of 10 helper for LCD_ShowNum */
static u32 lcd_pow(u8 m, u8 n) { u32 r = 1; while (n--) r *= m; return r; }

/* show one ASCII char. size = 12 / 16 / 24. mode: 0 opaque, 1 overlay */
void LCD_ShowChar(u16 x, u16 y, u8 chr, u8 size, u8 mode)
{
    const u8 *pfont;
    u8  bpr, width, row, b, bit;
    u8  temp;

    if (chr < ' ' || chr > '~') chr = ' ';
    chr -= ' ';

    if (size == 12)      { pfont = asc2_1206[chr]; bpr = 1; width = 6;  }
    else if (size == 24) { pfont = asc2_2412[chr]; bpr = 2; width = 12; }
    else                 { size = 16; pfont = asc2_1608[chr]; bpr = 1; width = 8; }

    for (row = 0; row < size; row++)
    {
        for (b = 0; b < bpr; b++)
        {
            temp = pfont[row * bpr + b];
            for (bit = 0; bit < 8; bit++)
            {
                u8 px = b * 8 + bit;
                if (px >= width) break;
                if (temp & 0x80)
                    LCD_Fast_DrawPoint(x + px, y + row, POINT_COLOR);
                else if (mode == 0)
                    LCD_Fast_DrawPoint(x + px, y + row, BACK_COLOR);
                temp <<= 1;
            }
        }
    }
}

/* show an unsigned number, len digits, leading zeros suppressed */
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
    u8 t, temp, enshow = 0;
    u8 cw = (size == 12) ? 6 : (size == 24 ? 12 : 8);
    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0) { LCD_ShowChar(x + cw * t, y, ' ', size, 0); continue; }
            else enshow = 1;
        }
        LCD_ShowChar(x + cw * t, y, temp + '0', size, 0);
    }
}

/* show a string until end or width/height bounds reached */
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, char *p)
{
    u8 cw = (size == 12) ? 6 : (size == 24 ? 12 : 8);
    u16 x0 = x;
    width += x;
    height += y;
    while (*p && *p != '\0')
    {
        if (x >= width)  { x = x0; y += size; }
        if (y >= height) break;
        LCD_ShowChar(x, y, *p, size, 0);
        x += cw;
        p++;
    }
}

/* ---- HX8357D power-on initialization sequence ---- */
void LCD_Init(void)
{
    LCD_GPIO_Init();
    delay_ms(50);

    LCD_WR_REG(0xB9);            /* SETEXTC: enable extension command */
    LCD_WR_DATA(0xFF); LCD_WR_DATA(0x83); LCD_WR_DATA(0x57);

    LCD_WR_REG(0xB6);           /* SETVCOM */
    LCD_WR_DATA(0x2C);

    LCD_WR_REG(0x11);           /* sleep out */
    delay_ms(120);

    LCD_WR_REG(0xB3);           /* RGB interface off, internal clock */
    LCD_WR_DATA(0x43); LCD_WR_DATA(0x00); LCD_WR_DATA(0x06); LCD_WR_DATA(0x06);

    LCD_WR_REG(0xB1);           /* SETPOWER */
    LCD_WR_DATA(0x00); LCD_WR_DATA(0x15); LCD_WR_DATA(0x1C);
    LCD_WR_DATA(0x1C); LCD_WR_DATA(0x83); LCD_WR_DATA(0xAA);

    LCD_WR_REG(0xC0);           /* SETSTBA */
    LCD_WR_DATA(0x50); LCD_WR_DATA(0x50); LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x3C); LCD_WR_DATA(0x1E); LCD_WR_DATA(0x08);

    LCD_WR_REG(0xB4);           /* SETCYC */
    LCD_WR_DATA(0x02); LCD_WR_DATA(0x40); LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x2A); LCD_WR_DATA(0x2A); LCD_WR_DATA(0x0D); LCD_WR_DATA(0x78);

    LCD_WR_REG(0xE0);           /* SETGAMMA */
    LCD_WR_DATA(0x02); LCD_WR_DATA(0x0A); LCD_WR_DATA(0x11); LCD_WR_DATA(0x1D);
    LCD_WR_DATA(0x23); LCD_WR_DATA(0x35); LCD_WR_DATA(0x41); LCD_WR_DATA(0x4B);
    LCD_WR_DATA(0x4B); LCD_WR_DATA(0x42); LCD_WR_DATA(0x3A); LCD_WR_DATA(0x27);
    LCD_WR_DATA(0x1B); LCD_WR_DATA(0x08); LCD_WR_DATA(0x09); LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x02); LCD_WR_DATA(0x0A); LCD_WR_DATA(0x11); LCD_WR_DATA(0x1D);
    LCD_WR_DATA(0x23); LCD_WR_DATA(0x35); LCD_WR_DATA(0x41); LCD_WR_DATA(0x4B);
    LCD_WR_DATA(0x4B); LCD_WR_DATA(0x42); LCD_WR_DATA(0x3A); LCD_WR_DATA(0x27);
    LCD_WR_DATA(0x1B); LCD_WR_DATA(0x08); LCD_WR_DATA(0x09); LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x00); LCD_WR_DATA(0x01);

    LCD_WR_REG(0x3A);           /* COLMOD: 16 bit/pixel */
    LCD_WR_DATA(0x55);

    LCD_WR_REG(0x36);           /* MADCTL: landscape, RGB order */
    LCD_WR_DATA(0xA0);

    LCD_WR_REG(0x29);           /* display on */
    delay_ms(50);

    lcddev.id     = 0x8357;
    lcddev.dir    = 1;
    lcddev.width  = 480;
    lcddev.height = 320;

    LCD_BL_ON();
    POINT_COLOR = BLACK;
    BACK_COLOR  = WHITE;
    LCD_Clear(WHITE);
}
