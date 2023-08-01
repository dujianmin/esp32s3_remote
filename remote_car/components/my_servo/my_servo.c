#include <stdio.h>
#include "my_servo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "config.h"
#include "esp_log.h"
static const char *TAG = "MY_SERVO";

// You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH_US (1000) // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US (2000) // Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE (90)          // Maximum angle in degree upto which servo can rotate

#define SERVO_PULSE_GPIO (18) // GPIO connects to the PWM signal line

static inline uint32_t example_convert_servo_angle_to_duty_us(int angle)
{
    return (angle + SERVO_MAX_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (2 * SERVO_MAX_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}


// 任务函数
void servoTask(void* pvParameters)
{
    MotorParams *params = (MotorParams *)pvParameters;  


    while (1) {
        int angle = params->angle;
        ESP_LOGI(TAG, "angle - a: %d", angle);
        // 初始化 MCPWM 和 GPIO
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PULSE_GPIO);

        mcpwm_config_t pwm_config = {
            .frequency = 50,
            .cmpr_a = 0,
            .counter_mode = MCPWM_UP_COUNTER,
            .duty_mode = MCPWM_DUTY_MODE_0,
        };
        mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

        // vTaskDelay(pdMS_TO_TICKS(100));

        // 控制舵机角度
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, example_convert_servo_angle_to_duty_us(angle)));

        vTaskDelay(pdMS_TO_TICKS(100)); // 每隔1秒执行一次
    }
}