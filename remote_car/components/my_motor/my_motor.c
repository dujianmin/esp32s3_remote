#include <stdio.h>
#include "my_motor.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// #include "config.h"
// 48 19 20 6
#define TAG "MY_MOTOR"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE

#define LEDC_OUTPUT_IO_0 (48) // Define the output GPIO
#define LEDC_CHANNEL_DC0 LEDC_CHANNEL_0
#define LEDC_OUTPUT_IO_1 (19) // Define the output GPIO
#define LEDC_CHANNEL_DC1 LEDC_CHANNEL_1

#define LEDC_OUTPUT_IO_2 (6) // Define the output GPIO
#define LEDC_CHANNEL_DC2 LEDC_CHANNEL_2
#define LEDC_OUTPUT_IO_3 (20) // Define the output GPIO
#define LEDC_CHANNEL_DC3 LEDC_CHANNEL_3

#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
// #define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY (5000) // Frequency in Hertz. Set frequency at 5 kHz

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 5 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel_left_1 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_DC0,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO_0,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_left_1));
    ledc_channel_config_t ledc_channel_left_2 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_DC1,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO_1,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_left_2));

    ledc_channel_config_t ledc_channel_right_1 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_DC2,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO_2,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_right_1));
    ledc_channel_config_t ledc_channel_right_2 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_DC3,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO_3,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_right_2));
}

static void motor_init(void)
{
    example_ledc_init();
}
void motor_control_task(void *pvParameters)
{

    motor_init();
    MotorParams *params = (MotorParams *)pvParameters;

    while (1)
    {

        // 获取left_speed和right_speed的值，可以通过队列或者全局变量获取
        int left_speed = params->left_speed;
        int right_speed = params->right_speed;
        ESP_LOGI(TAG, "motor speed - left: %d, right: %d", left_speed, right_speed);
        if (left_speed > 0)
        {
            int result = ((1u << 13) - 1) * left_speed / 100;
            // Set duty to 50%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC0, result));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC0));

            // Set duty to 0%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC1, 0));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC1));
        }
        else
        {
            int result = -(((1 << 13) - 1) * left_speed) / 100;
            // Set duty to 50%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC0, 0));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC0));

            // Set duty to 0%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC1, result));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC1));
        }
        if (right_speed > 0)
        {
            int result = ((1u << 13) - 1) * right_speed / 100;
            // Set duty to 50%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC2, result));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC2));

            // Set duty to 0%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC3, 0));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC3));
        }
        else
        {
            int result = -(((1 << 13) - 1) * right_speed) / 100;
            // Set duty to 50%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC2, 0));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC2));

            // Set duty to 0%
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_DC2, result));
            // Update duty to apply the new value
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_DC3));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}