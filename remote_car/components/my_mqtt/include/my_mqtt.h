#ifndef MY_MQTT
#define MY_MQTT

#include <stdio.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include <cJSON.h>

extern esp_mqtt_client_handle_t client;
void mqtt_task(void *pvParameters);

#endif
