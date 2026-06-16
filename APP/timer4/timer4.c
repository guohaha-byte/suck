#include "timer4.h"

/* ==================== TIM4初始化函数 ==================== */

/**
 * @brief  TIM4定时器初始化，配置为1ms中断一次
 * @note   STM32F407 TIM4挂载在APB1总线
 *         APB1时钟=42MHz，APB1定时器时钟=84MHz
 *         84MHz / (83+1) / (999+1) = 1000Hz -> 1ms中断
 */
void TIM4_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

    /* 1. 使能TIM4时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    /* 2. 定时器基本参数配置 */
    TIM_TimeBaseStructure.TIM_Period        = 999;   // 自动重装值(ARR)，计数0~999共1000次
    TIM_TimeBaseStructure.TIM_Prescaler     = 83;    // 预分频(PSC)，84MHz/(83+1)=1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    // 计时周期 = 1MHz / 1000 = 1kHz = 1ms 中断一次

    /* 3. 使能TIM4更新中断 */
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    /* 4. NVIC中断优先级配置 */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0; // 子优先级0
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 5. 启动TIM4 */
    TIM_Cmd(TIM4, ENABLE);
}



