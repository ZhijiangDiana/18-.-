/*
 * motor.c
 *
 *  Created on: 2023年4月18日
 *      Author: Lenovo
 */
#include "motor.h"

int16 pulseCount_1, pulseCount_2;
int32 motorPWML=800, motorPWMR=800;

void motor_init(void)
{
    //建议电磁组电机频率选用13K-17K
    //最大占空比值PWM_DUTY_MAX 可以在zf_driver_pwm.h文件中修改 默认为10000

    gpio_init(MOTOR1_A, GPO, 0, GPIO_PIN_CONFIG);
    pwm_init(MOTOR1_B, 17000, 0);
    gpio_init(MOTOR2_A, GPO, 0, GPIO_PIN_CONFIG);
    pwm_init(MOTOR2_B, 17000, 0);
}

#define SPEED_RECORD_NUM 50
float speed_Record1[SPEED_RECORD_NUM]={0};
float speed_Record2[SPEED_RECORD_NUM]={0};
int16 Speed_Low_Filter(float new_Spe,float *speed_Record)
{
    float sum = 0.0f;
    for(uint8_t i=SPEED_RECORD_NUM-1;i>0;i--)//将现有数据后移一位
    {
        speed_Record[i] = speed_Record[i-1];
        sum += speed_Record[i-1];
    }
    speed_Record[0] = new_Spe;//第一位是新的数据
    sum += new_Spe;
    return (int16)sum/SPEED_RECORD_NUM;//返回均值
}

void getPulseCount(){
    pulseCount_1 = abs(encoder_get_count(ENCODER_1)/5);// 获取编码器计数
    pulseCount_2 = abs(encoder_get_count(ENCODER_2)/5);// 获取编码器计数
    encoder_clear_count(ENCODER_1);// 清空编码器计数
    encoder_clear_count(ENCODER_2);// 清空编码器计数

    pulseCount_1 = Speed_Low_Filter(pulseCount_1, speed_Record1);
    pulseCount_2 = Speed_Low_Filter(pulseCount_2, speed_Record2);
}

void motor_control(int32 duty_1, int32 duty_2)
{
    //对占空比限幅
    if(duty_1>PWM_DUTY_MAX)         duty_1 = PWM_DUTY_MAX;
    else if(duty_1<-PWM_DUTY_MAX)   duty_1 = -PWM_DUTY_MAX;

    if(duty_2>PWM_DUTY_MAX)         duty_2 = PWM_DUTY_MAX;
    else if(duty_2<-PWM_DUTY_MAX)   duty_2 = -PWM_DUTY_MAX;


    if(0<=duty_1)
    {
        gpio_set_level(MOTOR1_A, 0);
        pwm_set_duty(MOTOR1_B, duty_1);
    }
    else
    {
        gpio_set_level(MOTOR1_A, 1);
        pwm_set_duty(MOTOR1_B, -duty_1);
    }

    if(0<=duty_2)
    {
        gpio_set_level(MOTOR2_A, 0);
        pwm_set_duty(MOTOR2_B, duty_2);
    }
    else
    {
        gpio_set_level(MOTOR2_A, 1);
        pwm_set_duty(MOTOR2_B, -duty_2);
    }
}



_pid motor_pid;

/**
  * @brief  PID参数初始化
    *   @note   无
  * @retval 无
  */
void PID_param_init()
{
  /* 初始化参数 */
  motor_pid.target_val=25;
  motor_pid.actual_val=0.0;
  motor_pid.err = 0.0;
  motor_pid.err_last = 0.0;
  motor_pid.err_next = 0.0;

  motor_pid.Kp = 0.06;
  motor_pid.Ki = 0.000038;
  motor_pid.Kd = 0;//.05;
}

/**
  * @brief  设置目标值
  * @param  val     目标值
    *   @note   无
  * @retv0al 无
  */
void set_pid_target(float temp_val)
{
  motor_pid.target_val = temp_val;    // 设置当前的目标值
}

/**
  * @brief  获取目标值
  * @param  无
    *   @note   无
  * @retval 目标值
  */
float get_pid_target(void)
{
  return motor_pid.target_val;    // 设置当前的目标值
}

/**
  * @brief  设置比例、积分、微分系数
  * @param  p：比例系数 P
  * @param  i：积分系数 i
  * @param  d：微分系数 d
    *   @note   无
  * @retval 无
  */
void set_p_i_d(float p, float i, float d)
{
  motor_pid.Kp = p;    // 设置比例系数 P
  motor_pid.Ki = i;    // 设置积分系数 I
  motor_pid.Kd = d;    // 设置微分系数 D
}

/**
  * @brief  PID算法实现
  * @param  val     目标值
    *   @note   无
  * @retval 通过PID计算后的输出
  */
float PID_realize(float temp_val)
{
    /*计算目标值与实际值的误差*/
    motor_pid.err = motor_pid.target_val - temp_val;

    /*PID算法实现*/
    motor_pid.actual_val += motor_pid.Kp * (motor_pid.err - motor_pid.err_next)
                 +  motor_pid.Ki *  motor_pid.err
                 +  motor_pid.Kd * (motor_pid.err - 2 * motor_pid.err_next + motor_pid.err_last);
    /*传递误差*/
    motor_pid.err_last = motor_pid.err_next;
    motor_pid.err_next = motor_pid.err;

    /*返回当前实际值*/
    return motor_pid.actual_val;
}
