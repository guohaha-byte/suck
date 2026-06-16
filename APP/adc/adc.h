#ifndef _adc_H
#define _adc_H

#include "system.h"

/* Light sensor LS1 is wired to PF8 / ADC3_IN6 on the PuZhong QiLin F407 board.
 * Net name on schematic: LSENS / LIGHT.
 */
#define LSENS_ADC          ADC3
#define LSENS_ADC_CH       ADC_Channel_6   /* ADC3_IN6 */
#define LSENS_GPIO_PORT    GPIOF
#define LSENS_GPIO_PIN     GPIO_Pin_8
#define LSENS_GPIO_RCC     RCC_AHB1Periph_GPIOF

void Lsens_Init(void);          /* init ADC3 + PF8 in analog mode            */
u16  Lsens_Get_Adc(void);       /* raw 12-bit ADC value (0..4095)            */
u8   Lsens_Get_Val(void);       /* light intensity as percentage 0..100     */

#endif
