#include "my_mqtt.h"
#include "my_motor.h"
#include "my_servo.h"
#include "joystick.h"
#include "cst816t.h"
#include "my_remote.h"

#define TAG "MAIN_REMOTE_CAR"

MotorParams motor_params;
void app_main()
{
    motor_params.left_speed = 0;
    motor_params.right_speed = 0;
    motor_params.angle = 0;   
    //音频视频通信任务
    xTaskCreatePinnedToCore(my_rtc_task_run, "my_rtc_task", 2048 * 2, NULL, 5, NULL, 1);

    //通信协议任务
    xTaskCreatePinnedToCore(mqtt_task, "mqtt_task", 2048 * 2, &motor_params, 5, NULL, 1);   
    //接受控制指令任务
    xTaskCreate(motor_control_task, "motor_control_task", 2048*2, &motor_params, 5, NULL);    
    xTaskCreate(servoTask, "servoTask", 2048*2, &motor_params, 5, NULL);
    //发布控制指令任务
    xTaskCreate(cst816t_test_task, "cst816t_test_task", 1024 * 2, NULL, 10, NULL);   
    xTaskCreatePinnedToCore(joystick_task, "joystick_task", 2048 * 2, NULL, 5, NULL, 1);        
    while (1)
    {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
