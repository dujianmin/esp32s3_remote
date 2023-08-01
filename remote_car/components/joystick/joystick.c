#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "joystick.h"
#include "my_mqtt.h"

#include "cJSON.h"

#define JOYSTICK_X_CHANNEL ADC2_CHANNEL_8
#define JOYSTICK_Y_CHANNEL ADC2_CHANNEL_9
#define JOYSTICK_X_GPIO 19
#define JOYSTICK_Y_GPIO 20

void joystick_task(void *pvParameters) {
    // 配置ADC通道
    // adc2_config_width(ADC_WIDTH_BIT_12);
    adc2_config_channel_atten(JOYSTICK_X_CHANNEL, ADC_ATTEN_DB_11);
    adc2_config_channel_atten(JOYSTICK_Y_CHANNEL, ADC_ATTEN_DB_11);

    // 配置GPIO
    gpio_pad_select_gpio(JOYSTICK_X_GPIO);
    gpio_pad_select_gpio(JOYSTICK_Y_GPIO);
    gpio_set_direction(JOYSTICK_X_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(JOYSTICK_Y_GPIO, GPIO_MODE_INPUT);
    int x_value = 0;
    int y_value = 0;
    int angle = 90;
    char xy_str[30];  // 定义一个足够大的字符数组

    while (1) {
    // 读取X轴和Y轴的ADC值

    adc2_get_raw(JOYSTICK_X_CHANNEL, ADC_WIDTH_BIT_12, &x_value);
    adc2_get_raw(JOYSTICK_Y_CHANNEL, ADC_WIDTH_BIT_12, &y_value);
    //假设一个角度

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "X", x_value);
    cJSON_AddNumberToObject(root, "Y", y_value);
    cJSON_AddNumberToObject(root, "A", angle);
    char *json_str = cJSON_Print(root);

    // sprintf(xy_str, "X: %d, Y: %d", x_value, y_value);  // 将"x_value"和"y_value"拼接到一起
    // 输出结果
    printf("Joystick X: %d, Y: %d, A: %d\n", x_value, y_value, angle);
    int msg_id = esp_mqtt_client_publish(client, "/topic/ps4_2", json_str, 0, 0, 0);

    cJSON_Delete(root);
    free(json_str);    
    // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        vTaskDelay(pdMS_TO_TICKS(3000));  // 延时50毫秒
    }
}
