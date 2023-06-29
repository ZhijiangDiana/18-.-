#include "zf_common_headfile.h"
#include "servo.h"
#include "motor.h"
#include "laiqu.h"
#include "tof.h"
#include "hall_stopline.h"
#include "Read_ADC.h"
#include "judgement.h"
#include "pit.h"
#include "zf_device_virtual_oscilloscope.h"
#include "config.h"

void img_handler();

// 避障相关
uint16 distance=0;
uint16 obstacle_cnt=0;
uint8 obstacle_phase=0;

int main (void)
{

    clock_init(SYSTEM_CLOCK_144M);                                              // 初始化芯片时钟 工作频率为 144MHz
    debug_init();                                                               // 初始化默认 Debug UART
    servo_init();
    motor_init();
    mt9v03x_init();
    ips200_init(IPS200_TYPE_PARALLEL8);
    ADC_init();
    tofInit();

    timer_init(TIM_5, TIMER_MS);//计时器，查看程序运行时间

    encoder_quad_init(ENCODER_1, ENCODER_1_A, ENCODER_1_B);                     // 初始化编码器模块与引脚 正交解码编码器模式
    encoder_quad_init(ENCODER_2, ENCODER_2_A, ENCODER_2_B);                     // 初始化编码器模块与引脚 正交解码编码器模式

    ImagePerspective_Init();

#if MOTOR_DEBUG_STATUS
//    uart_init(UART_3, 115200, UART3_MAP0_TX_B10, UART3_MAP0_RX_B11);
    bluetooth_ch9141_init();
    key_init(5);
#if CAR_TYPE
    float target_pulse[3] = {25, 45, 65};
#else
    float target_pulse[3] = {10, 30, 50};
#endif
    uint8 key_status = 0;               // 0电机目标速度调整，1pid参数调整
    uint8 key_ChangeWhichOf_pid = 0;    // 0调整p，1调整i,2调整d
#endif

    PID_param_init();
    // 1m/s 54pulse/5ms
    set_pid_target(25);

    Main_pit_init();


    while(1)
    {
        // 此处编写需要循环执行的代码
#if MOTOR_DEBUG_STATUS
        key_scanner();
        if(key_status)
        {// change motor target pulse
            switch(key_get_state(KEY_1))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    set_pid_target(target_pulse[0]);
                    break;
            }
            switch(key_get_state(KEY_2))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    set_pid_target(target_pulse[1]);
                    break;
            }
            switch(key_get_state(KEY_3))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    set_pid_target(target_pulse[2]);
                    break;
            }
        }
        else
        {// change motor pid
            switch(key_get_state(KEY_1))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    if(key_ChangeWhichOf_pid==0)
                        pid.Kp+=0.001;
                    else if(key_ChangeWhichOf_pid==1)
                        pid.Ki+=0.000001;
                    else if(key_ChangeWhichOf_pid==2)
                        pid.Kd+=0.01;
                    break;
            }
            switch(key_get_state(KEY_2))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    if(key_ChangeWhichOf_pid==0)
                        pid.Kp-=0.001;
                    else if(key_ChangeWhichOf_pid==1)
                        pid.Ki-=0.000001;
                    else if(key_ChangeWhichOf_pid==2)
                        pid.Kd-=0.01;
                    break;
            }
            switch(key_get_state(KEY_3))
            {
                case KEY_SHORT_PRESS:
                case KEY_LONG_PRESS:
                    key_ChangeWhichOf_pid++;
                    key_ChangeWhichOf_pid%=3;
                    break;
            }
        }

        switch(key_get_state(KEY_4))
        {
            case KEY_SHORT_PRESS:
            case KEY_LONG_PRESS:
                key_status++;
                key_status%=2;
                break;
        }
#endif
        if(mt9v03x_finish_flag)
        {
            img_handler();
            mt9v03x_finish_flag = 0;
        }
        // 此处编写需要循环执行的代码
    }

}

void ips200_show()
{
    ips200_show_gray_image(0,0,_img[0],RESULT_COL,RESULT_ROW,RESULT_COL,RESULT_ROW,0);
//    camera_send_image(DEBUG_UART_INDEX, (const uint8 *)bin_image, MT9V03X_IMAGE_SIZE);
//    bluetooth_ch9141_send_image((const uint8 *)bin_image, MT9V03X_IMAGE_SIZE);
    Draw_Side();

    ips200_show_string(110, 0, "Angle:");
    ips200_show_float(110, 20, Angle, 5, 2);

    ips200_show_string(110,40,"Distance: ");
    ips200_show_int(150,40,distance,5);
    ips200_show_string(110,60,"Voltage: ");
    ips200_show_float(150,60,voltage_now,2,3);

    // motor pid
#if MOTOR_DEBUG_STATUS
    ips200_show_string(0, 80, "pulse count:");
    ips200_show_int(100, 80, pulseCount_1, 5);
    ips200_show_int(150, 80, pulseCount_2, 5);
    ips200_show_string(0, 100, "motor:");
    ips200_show_int(70, 100, motorPWML, 5);
    ips200_show_int(120, 100, motorPWMR, 5);
    ips200_show_string(0, 120, "target pulse:");
    ips200_show_int(100, 120, get_pid_target(), 3);

    ips200_show_string(0, 140, "Kp");
    ips200_show_float(40, 140, pid.Kp, 2, 8);
    ips200_show_string(0, 140, "Ki");
    ips200_show_float(40, 140, pid.Ki, 2, 8);
    ips200_show_string(0, 140, "Kd");
    ips200_show_float(40, 140, pid.Kd, 2, 8);

#else
    ips200_show_string(0, 150, "elec ADC value:");
    ips200_show_int(0, 170, adc_LL, 5);
    ips200_show_int(40, 170, adc_L, 5);
    ips200_show_int(80, 170, adc_R, 5);
    ips200_show_int(120, 170, adc_RR, 5);

    // camera data
    ips200_show_string(0, 190, "corner point:");
    ips200_show_int(0, 210, LeftBreakpoint.end_y, 4);
    ips200_show_int(40, 210, RightBreakpoint.end_y, 4);
    ips200_show_int(0, 230, LeftBreakpoint.start_y, 4);
    ips200_show_int(40, 230, RightBreakpoint.start_y, 4);

//    ips200_draw_line(0, aimLine, RESULT_COL-1, aimLine, RGB565_GREEN);
//    ips200_show_int(0, 80, road_width[aimLine], 5);
//    ips200_show_int(0, 100, leftline[aimLine], 5);
//    ips200_show_int(50, 100, rightline[aimLine], 5);

    ips200_show_string(0, 250, "Status");
    ips200_show_int(100, 250, circle_status, 5);
    if(cross_flag)
        ips200_show_string(0, 270, "cross");
    else if(!cross_flag)
        ips200_show_string(0, 270, "     ");
    ips200_show_int(100, 270, cross_cnt, 6);

#endif
}

void elec_handler()
{
    // adc+encoder+scope...<200us

    Read_ADC();

    static uint32 elec_handler_cnt = 0;// 通过count计数实现delay效果
    if(elec_handler_cnt%Delay_cnt_calc(5)==0)
    {
        getPulseCount();

#if MOTOR_DEBUG_STATUS
        virtual_oscilloscope_data_conversion(pulseCount_1,pulseCount_2,0,0);
//        uart_write_buffer(UART_3, virtual_oscilloscope_data, 10);
        bluetooth_ch9141_send_buff(virtual_oscilloscope_data, 10);
#endif

        motorPWML += PID_realize(pulseCount_1);
        motorPWMR += PID_realize(pulseCount_2);

        if(motorPWML>2000)
            motorPWML = 2000;
        if(motorPWMR>2000)
            motorPWMR=2000;

        motor_control(motorPWML, motorPWMR);
    }

    // 红外避障
    if (!obstacle_flag && elec_handler_cnt % Delay_cnt_calc(100) == 0)
    {
        // 红外测距对不同的颜色的障碍物敏感度不同,对红色障碍物测量值偏大，对蓝色障碍物测量值偏小
        distance = Get_Distance();
//        printf("Dis: %d",distance);
        if (distance <= 600)
        {
//            printf("Obstacle!");
            obstacle_flag=true;
            obstacle_cnt=0;
        }
    }

    if(!left_circle_flag && !right_circle_flag && !obstacle_flag)
    {
        judgement();
        CURRENT_STATUS = Status_Common;
    }
    else if(left_circle_flag || right_circle_flag)
    {
        static uint16 cnt=0;
        if(left_circle_flag)
        {
            switch (circle_status)
            {
                case 1:// step1 避开第一个断口(正常巡线应该就行)
                    CURRENT_STATUS = Status_Common;

                    if(++cnt>Delay_cnt_calc(1000) && adc_LL>circle_threshold)
                    {
                        circle_status++;
                        cnt = 0;
                        break;
                    }

                    // 如果一边大了之后，两边同时都大了，说明误判了，当前应该是十字
                    if(adc_LL>circle_threshold && adc_RR>circle_threshold)
                    {
                        circle_status = 0;
                        cnt = 0;
                        left_circle_flag = false;
                        cross_cnt = 0;
                        cross_flag = true;
                    }
                    break;
                case 2:// step2 第二个断口入环，强行扭头入环
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(100));
                    if(++cnt>Delay_cnt_calc(500))
                    {
                        circle_status++;
                        cnt = 0;
                    }
                    break;
                case 3:// step3 正常巡线
                    CURRENT_STATUS = Status_Common;
                    if(adc_RR>circle_threshold)
                    {
                        circle_status++;
                    }
                    break;
                case 4:// step4 出环（正常巡线应该可以）
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(98));
                    if(++cnt>Delay_cnt_calc(750))// && adc_LL<circle_threshold
                    {
                        circle_status++;
                        cnt = 0;
                    }
                    break;
                case 5:// step5 出环后
                    CURRENT_STATUS = Status_Common;
                    if(++cnt>Delay_cnt_calc(500))
                    {
                        cnt = 0;
                        circle_status = 0;
                        left_circle_flag = false;
                    }
                    break;
            }
        }
        else
        {
            switch (circle_status)
            {
                case 1:// step1 避开第一个断口(正常巡线应该就行)
                    CURRENT_STATUS = Status_Common;

                    if(++cnt>Delay_cnt_calc(1000) && adc_RR>circle_threshold)
                    {
                        circle_status++;
                        cnt = 0;
                        break;
                    }

                    // 如果一边大了之后，两边同时都大了，说明误判了，当前应该是十字
                    if(adc_LL>circle_threshold && adc_RR>circle_threshold)
                    {
                        circle_status = 0;
                        cnt = 0;
                        right_circle_flag = false;
                        cross_cnt = 0;
                        cross_flag = true;
                    }
                    break;
                case 2:// step2 第二个断口入环，强行扭头入环
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(82));
                    if(++cnt>Delay_cnt_calc(750))
                    {
                        circle_status++;
                        cnt = 0;
                    }
                    break;
                case 3:// step3 正常巡线
                    CURRENT_STATUS = Status_Common;
                    if(adc_LL>circle_threshold)
                    {
                        circle_status++;
                    }
                    break;
                case 4:// step4 出环（正常巡线应该可以）
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(82));
                    if(++cnt>Delay_cnt_calc(1000))
                    {
                        circle_status++;
                        cnt = 0;
                    }
                    break;
                case 5:// step5 出环后
                    CURRENT_STATUS = Status_Common;
                    if(++cnt>Delay_cnt_calc(500))
                    {
                        cnt = 0;
                        circle_status = 0;
                        right_circle_flag = false;
                    }
                    break;
            }
        }
    }
    else if(obstacle_flag)
    {
        set_pid_target(15);
        CURRENT_STATUS = Status_Stop;
        switch (obstacle_phase)
        {
            case 0:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(75));
                if(obstacle_cnt>=Delay_cnt_calc(180)){
                    obstacle_cnt=0;
                    obstacle_phase=1;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 1:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(90));
                if(obstacle_cnt>=Delay_cnt_calc(600)){
                    obstacle_cnt=0;
                    obstacle_phase=2;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 2:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(105));
                if(obstacle_cnt>=Delay_cnt_calc(200)){
                    obstacle_cnt=0;
                    obstacle_phase=3;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 3:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(90));
                if(obstacle_cnt>=Delay_cnt_calc(300)){
                    obstacle_cnt=0;
                    obstacle_phase=4;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 4:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(105));
                if(obstacle_cnt>=Delay_cnt_calc(200)){
                    obstacle_cnt=0;
                    obstacle_phase=5;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 5:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(90));
                if(obstacle_cnt>=Delay_cnt_calc(600)){
                    obstacle_cnt=0;
                    obstacle_phase=6;
                }else{
                    obstacle_cnt++;
                }
                break;
            case 6:
                pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(75));
                if(obstacle_cnt>=Delay_cnt_calc(180)){
                    obstacle_cnt=0;
                    obstacle_phase=0;
                    obstacle_flag=false;
                }else{
                    obstacle_cnt++;
                }
                break;
        }
    }

    //电池电量检测
    if(elec_handler_cnt%Delay_cnt_calc(5000)){
        Get_Battery_Voltage();
    }

    if (elec_handler_cnt == Delay_cnt_calc(5000)) {
        elec_handler_cnt = 0;
    }else{
        elec_handler_cnt++;
    }

}

void img_handler()
{
    // 开始计时
    timer_start(TIM_5);

    ImageProcess();
    // 结束计时
    timer_stop(TIM_5);
    ips200_show_string(0, 290, "imgproc time:");
    ips200_show_int(100, 290, (int16)timer_get(TIM_5), 8);
    timer_clear(TIM_5);

    ips200_show();
}
