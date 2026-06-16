#ifndef _mqtt_H
#define _mqtt_H

#include "system.h"

/* Publish an MQTT PUBLISH packet (QoS0) with the given topic and ASCII
 * payload through the ESP8266 (AT+CIPSEND). Remaining length must be < 128. */
void Mqtt_Publish(const char *topic, const char *payload);

/* Parse an incoming command buffer (the ESP8266 +IPD frame) looking for
 * JSON fields "led1" / "led2" with value 0 or 1. Updates *led1 / *led2 when
 * present. Returns 1 if at least one LED state was found, else 0. */
u8 Mqtt_ParseCmd(const char *buf, u16 len, u8 *led1, u8 *led2);

#endif
