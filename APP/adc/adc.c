#include "adc.h"
#include "SysTick.h"

/*******************************************************************************
 * Light sensor (photo-resistor LS1) acquisition via ADC3 channel 6 (PF8).
 *
 * Hardware: LS1 forms a voltage divider with a fixed resistor to VCC3.3.
 * The brighter the environment, the lower the measured ADC voltage, so the
 * percentage is inverted to make "brighter => larger value".
 *   Vadc = adc * 3.3 / 4096
 ******************************************************************************/

/*******************************************************************************
 * Lsens_Init : configure PF8 as analog input and ADC3 in single conversion,
 *              software triggered mode.
 ******************************************************************************/
void Lsens_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    /* clocks: GPIOF + ADC3 */
    RCC_AHB1PeriphClockCmd(LSENS_GPIO_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

    /* PF8 analog input, no pull */
    GPIO_InitStructure.GPIO_Pin  = LSENS_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LSENS_GPIO_PORT, &GPIO_InitStructure);

    /* ADC common: independent mode, PCLK2/4 */
    ADC_CommonInitStructure.ADC_Mode             = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler        = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC3: 12-bit, single channel, software triggered */
    ADC_InitStructure.ADC_Resolution           = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode         = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode   = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign            = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion      = 1;
    ADC_Init(LSENS_ADC, &ADC_InitStructure);

    ADC_Cmd(LSENS_ADC, ENABLE);
}

/*******************************************************************************
 * Get_Adc : one software-triggered conversion of the given channel.
 ******************************************************************************/
static u16 Get_Adc(u8 ch)
{
    ADC_RegularChannelConfig(LSENS_ADC, ch, 1, ADC_SampleTime_480Cycles);
    ADC_SoftwareStartConv(LSENS_ADC);
    while (ADC_GetFlagStatus(LSENS_ADC, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(LSENS_ADC);
}

/*******************************************************************************
 * Lsens_Get_Adc : averaged raw value over several samples (noise filtering).
 ******************************************************************************/
u16 Lsens_Get_Adc(void)
{
    u32 sum = 0;
    u8  t;
    for (t = 0; t < 10; t++)
    {
        sum += Get_Adc(LSENS_ADC_CH);
        delay_ms(2);
    }
    return (u16)(sum / 10);
}

/*******************************************************************************
 * Lsens_Get_Val : light intensity in percent (0 = dark, 100 = bright).
 ******************************************************************************/
u8 Lsens_Get_Val(void)
{
    u16 adc = Lsens_Get_Adc();
    /* brighter => lower adc, so invert. 4095 -> 0%, 0 -> 100% */
    return (u8)(100 - (adc * 100 / 4095));
}
