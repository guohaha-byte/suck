#ifndef _gui_H
#define _gui_H

#include "system.h"

/* Build the static screen layout (title + field labels). */
void UI_Init(void);

/* Refresh dynamic values. Only changed fields are redrawn (anti-flicker).
 *   light : 0..100 (%)
 *   led1, led2, wifi, mqtt : 0 = off/disconnected, 1 = on/connected
 */
void UI_Update(u8 light, u8 led1, u8 led2, u8 wifi, u8 mqtt);

#endif
