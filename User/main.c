#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "timer4.h"
#include "usart.h"
#include "wifi_config.h"
#include "wifi_function.h"
#include "adc.h"
#include "lcd.h"
#include "gui.h"
#include "mqtt.h"
#include <string.h>
#include <stdio.h>
#include "stm32f4xx.h"

/* ===== user configuration : fill in before building ===== */
#define SSID          ""        /* WiFi SSID            */
#define PASD          ""        /* WiFi password        */
#define MQTTSV        ""        /* EMQX broker IP       */
#define MQTTSV_PORT   ""        /* broker port, e.g. 1883 */

#define TOPIC_PUB     "mcu"     /* publish light data here   */
#define TOPIC_SUB     "app"     /* subscribe to commands here */

/* ===================== global state ===================== */
u16 TX_1MS    = 0;      /* 1ms tick counter for 3s publish    */
u16 Adc_1MS   = 0;      /* 1ms tick counter for ADC sampling  */
u8  FlagUpDara = 0;     /* set every 3s -> time to publish     */
u8  FlagAdc    = 0;     /* set every 500ms -> sample light     */

u8  g_light    = 0;     /* current light intensity (0..100%)   */
u8  StateLED1  = 0;     /* LED1 (PF9) state: 0 off, 1 on       */
u8  StateLED2  = 0;     /* LED2 (PF10) state                   */
u8  g_wifi_ok  = 0;     /* WiFi connected flag                 */
u8  g_mqtt_ok  = 0;     /* MQTT connected flag                 */

/* ==================== MQTT CONNECT / SUBSCRIBE packets ==================== */
/* CONNECT : ClientID "1014238195", user "555798", pass "mcu", keepalive 120s */
char MqttDataDL[37] =
{
    0x10, 0x23,
    0x00, 0x04, 0x4D, 0x51, 0x54, 0x54,
    0x04, 0xC0, 0x00, 0x78,
    0x00, 0x0A, '1', 0x30, 0x31, 0x34, 0x32, 0x33,
    0x38, 0x31, 0x39, 0x35,
    0x00, 0x06, 0x35, 0x35, 0x35, 0x37, 0x39, 0x38,
    0x00, 0x03, 0x6D, 0x63, 0x75
};

/* SUBSCRIBE : topic "app", QoS 0 */
char MqttDataDY[10] =
{
    0x82, 0x08, 0x00, 0x02, 0x00, 0x03, 'a', 'p', 'p', 0x00
};

/* ==================== helpers ==================== */
void SendASC1(u8 d)
{
    USART_SendData(USART3, d);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/* send MQTT CONNECT */
void DengLu(void)
{
    u8 k;
    ESP8266_Cmd("AT+CIPSEND=37", ">", NULL, 2000);
    for (k = 0; k < 37; k++) SendASC1(MqttDataDL[k]);
    delay_ms(1000);
}

/* send MQTT SUBSCRIBE */
void DingYue(void)
{
    u8 k;
    ESP8266_Cmd("AT+CIPSEND=10", ">", NULL, 2000);
    for (k = 0; k < 10; k++) SendASC1(MqttDataDY[k]);
    delay_ms(1000);
}

/* apply LED states to the physical pins (active low) */
static void Apply_LED(void)
{
    LED1 = StateLED1 ? 0 : 1;
    LED2 = StateLED2 ? 0 : 1;
}

/* ====================== main ====================== */
int main(void)
{
    char json[48];

    /* --- system init --- */
    SysTick_Init(168);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    USART1_Init(115200);
    LED_Init();
    Lsens_Init();
    LCD_Init();
    UI_Init();
    UI_Update(g_light, StateLED1, StateLED2, g_wifi_ok, g_mqtt_ok);
    WiFi_Config();
    TIM4_Init();
    delay_ms(1000);

    /* --- ESP8266 bring-up --- */
    ESP8266_Choose(ENABLE);
    ESP8266_AT_Test();

    while (!ESP8266_Net_Mode_Choose(STA)) delay_ms(500);

    /* --- join WiFi --- */
    while (!ESP8266_JoinAP(SSID, PASD)) delay_ms(1000);
    g_wifi_ok = 1;
    UI_Update(g_light, StateLED1, StateLED2, g_wifi_ok, g_mqtt_ok);
    delay_ms(2000);

    /* --- connect to MQTT broker (TCP) --- */
    while (!ESP8266_Link_Server(enumTCP, MQTTSV, MQTTSV_PORT, Single_ID_0)) delay_ms(1000);
    delay_ms(2000);

    /* --- MQTT login + subscribe --- */
    DengLu();
    delay_ms(2000);
    DingYue();
    delay_ms(2000);
    g_mqtt_ok = 1;
    UI_Update(g_light, StateLED1, StateLED2, g_wifi_ok, g_mqtt_ok);

    /* ===================== main loop ===================== */
    while (1)
    {
        /* 1. sample light intensity every ~500ms */
        if (FlagAdc)
        {
            FlagAdc = 0;
            g_light = Lsens_Get_Val();
        }

        /* 2. parse any received command frame (JSON led1/led2) */
        if (strEsp8266_Fram_Record.InfBit.FramFinishFlag)
        {
            u16 len = strEsp8266_Fram_Record.InfBit.FramLength;
            if (len >= RX_BUF_MAX_LEN) len = RX_BUF_MAX_LEN - 1;
            strEsp8266_Fram_Record.Data_RX_BUF[len] = '\0';
            Mqtt_ParseCmd(strEsp8266_Fram_Record.Data_RX_BUF, len, &StateLED1, &StateLED2);
            strEsp8266_Fram_Record.InfBit.FramLength = 0;
            strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
        }

        /* 3. drive LEDs + refresh screen */
        Apply_LED();
        UI_Update(g_light, StateLED1, StateLED2, g_wifi_ok, g_mqtt_ok);

        /* 4. publish light data every 3s */
        if (FlagUpDara)
        {
            FlagUpDara = 0;
            sprintf(json, "{\"light\":%d,\"led1\":%d,\"led2\":%d}",
                    g_light, StateLED1, StateLED2);
            Mqtt_Publish(TOPIC_PUB, json);
        }
    }
}

/* ==================== TIM4 ISR : 1ms ==================== */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

        if (++TX_1MS >= 3000) { TX_1MS = 0; FlagUpDara = 1; }
        if (++Adc_1MS >= 500) { Adc_1MS = 0; FlagAdc = 1; }
    }
}

/* ==================== USART3 ISR : ESP8266 ==================== */
void USART3_IRQHandler(void)
{
    u8 ch;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        ch = (u8)USART_ReceiveData(USART3);
        if (strEsp8266_Fram_Record.InfBit.FramLength < (RX_BUF_MAX_LEN - 1))
            strEsp8266_Fram_Record.Data_RX_BUF[strEsp8266_Fram_Record.InfBit.FramLength++] = ch;
    }

    if (USART_GetITStatus(USART3, USART_IT_IDLE) == SET)
    {
        strEsp8266_Fram_Record.InfBit.FramFinishFlag = 1;
        ch = (u8)USART_ReceiveData(USART3);   /* clear IDLE: read SR then DR */
        (void)ch;
    }
}
