#include "mqtt.h"
#include "wifi_function.h"
#include "SysTick.h"
#include <string.h>
#include <stdio.h>

/* send one raw byte to the ESP8266 (USART3) */
static void Mqtt_SendByte(u8 d)
{
    USART_SendData(USART3, d);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

/*******************************************************************************
 * Mqtt_Publish : build & transmit a QoS0 PUBLISH packet.
 *   fixed header : 0x30, <remaining length>
 *   variable hdr : topic length (2 bytes) + topic
 *   payload      : ASCII JSON string
 ******************************************************************************/
void Mqtt_Publish(const char *topic, const char *payload)
{
    u8  pkt[160];
    u16 i = 0;
    u16 tlen = (u16)strlen(topic);
    u16 plen = (u16)strlen(payload);
    u16 remaining = 2 + tlen + plen;        /* assumes remaining < 128 */
    char cmd[24];
    u16 total;

    pkt[i++] = 0x30;
    pkt[i++] = (u8)remaining;
    pkt[i++] = (u8)(tlen >> 8);
    pkt[i++] = (u8)(tlen & 0xFF);
    memcpy(&pkt[i], topic, tlen);   i += tlen;
    memcpy(&pkt[i], payload, plen); i += plen;
    total = i;

    sprintf(cmd, "AT+CIPSEND=%d", total);
    ESP8266_Cmd(cmd, ">", NULL, 2000);

    for (i = 0; i < total; i++)
        Mqtt_SendByte(pkt[i]);

    delay_ms(300);
}

/* find the first '0' or '1' after the key, skipping ':' / spaces / quotes */
static int find_bit_after(const char *p, const char *end)
{
    while (p < end)
    {
        if (*p == '0' || *p == '1') return (*p - '0');
        if (*p == ',' || *p == '}') return -1;   /* value missing */
        p++;
    }
    return -1;
}

u8 Mqtt_ParseCmd(const char *buf, u16 len, u8 *led1, u8 *led2)
{
    const char *end = buf + len;
    const char *p;
    u8 found = 0;
    int v;

    for (p = buf; p + 4 <= end; p++)
    {
        if (p[0] == 'l' && p[1] == 'e' && p[2] == 'd')
        {
            if (p[3] == '1')
            {
                v = find_bit_after(p + 4, end);
                if (v >= 0) { *led1 = (u8)v; found = 1; }
            }
            else if (p[3] == '2')
            {
                v = find_bit_after(p + 4, end);
                if (v >= 0) { *led2 = (u8)v; found = 1; }
            }
        }
    }
    return found;
}
