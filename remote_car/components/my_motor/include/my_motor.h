#ifndef MY_MOTOR
#define MY_MOTOR
typedef struct {
    int left_speed;
    int right_speed;
    int angle;
} MotorParams;
void motor_control_task(void *pvParameters);

#endif