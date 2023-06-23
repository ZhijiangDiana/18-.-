#include "zf_common_headfile.h"
#include "servo.h"
#include "motor.h"
#include "laiqu.h"
#include "tof.h"
#include "hall_stopline.h"
#include "Read_ADC.h"
#include "judgement.h"
#include "pit.h"

int main (void)
{

    clock_init(SYSTEM_CLOCK_144M);                                              // 初始化芯片时钟 工作频率为 144MHz
    debug_init();                                                               // 初始化默认 Debug UART
    servo_init();
    motor_init();
    mt9v03x_init();
//    quinit();
//    laiinit();
//    tofinit();
//    hallinit();
    ips200_init(IPS200_TYPE_PARALLEL8);
    ADC_init();
    ALL_pit_init();

//    motor_control(600,600);

    while(1)
    {
        // 此处编写需要循环执行的代码

        // 此处编写需要循环执行的代码
    }

}

//extern uint8 cross_cnt;
void ips200_show()
{
//    ips200_show_gray_image(0, 0, bin_image[0], MT9V03X_W/2, MT9V03X_H/2, MT9V03X_W, MT9V03X_H, 0);
//    camera_send_image(DEBUG_UART_INDEX, (const uint8 *)bin_image, MT9V03X_IMAGE_SIZE/4);
//    Draw_Side();

    ips200_show_string(0, 130, "Angle:");
    ips200_show_float(50, 130, Angle, 5, 2);

    ips200_show_string(0, 150, "elec ADC value:");
    ips200_show_int(0, 170, adc_LL, 5);
    ips200_show_int(40, 170, adc_L, 5);
    ips200_show_int(80, 170, adc_R, 5);
    ips200_show_int(120, 170, adc_RR, 5);


//    ips200_show_int(0, 200, cross_cnt, 5);

//    ips200_show_string(0, 190, "corner point:");
//    ips200_show_int(0, 210, LeftBreakpoint.end_y, 4);
//    ips200_show_int(40, 210, RightBreakpoint.end_y, 4);
//    ips200_show_int(0, 230, LeftBreakpoint.start_y, 4);
//    ips200_show_int(40, 230, RightBreakpoint.start_y, 4);

    ips200_show_string(0, 250, "Status");
    ips200_show_int(100, 250, circle_status, 5);
    if(cross_flag)
        ips200_show_string(0, 270, "cross");
    else if(!cross_flag)
        ips200_show_string(0, 270, "     ");
}

void elec_handler()
{

    Read_ADC();

    if(!left_circle_flag && !right_circle_flag)
    {
        judgement();
//        servo_control(Status_Common);
        CURRENT_STATUS = Status_Common;
    }
    else
    {
        static int cnt=0;
        if(left_circle_flag)
        {
            switch (circle_status)
            {
                case 1:// step1 避开第一个断口(正常巡线应该就行)
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;

                    // 如果一边大了之后，两边同时都大了，说明误判了，当前应该是十字
                    if(adc_LL>circle_threshold && adc_RR>circle_threshold)
                    {
                        circle_status = 0;
                        cnt%=100;
                        left_circle_flag = false;
                        cross_cnt%=200;
                        cross_flag = true;
                    }

                    if(++cnt>100 && adc_LL>circle_threshold)// && current_err_circle>current_err_common
                    {
                        circle_status++;
                        cnt%=100;
                    }
                    break;
                case 2:// step2 第二个断口入环，强行扭头入环
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(98));
                    if(++cnt>200)
                    {
                        circle_status++;
                        cnt%=200;
                    }
                    break;
                case 3:// step3 正常巡线
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;
                    if(adc_RR>circle_threshold)
                    {
                        circle_status++;
                    }
                    break;
                case 4:// step4 出环（正常巡线应该可以）
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(98));
                    if(++cnt>200)// && adc_LL<circle_threshold
                    {
                        circle_status++;
                        cnt%=200;
                    }
                    break;
                case 5:// step5 出环后
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;
                    if(++cnt>200 && adc_LL<circle_threshold)
                    {
                        cnt%=200;
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
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;

                    // 如果一边大了之后，两边同时都大了，说明误判了，当前应该是十字
                    if(adc_LL>circle_threshold && adc_RR>circle_threshold)
                    {
                        circle_status = 0;
                        cnt%=100;
                        right_circle_flag = false;
                        cross_flag = true;
                        cross_cnt%=200;
                    }

                    if(++cnt>100 && adc_RR>circle_threshold)// && current_err_circle>current_err_common
                    {
                        circle_status++;
                        cnt%=100;
                    }
                    break;
                case 2:// step2 第二个断口入环，强行扭头入环
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(82));
                    if(++cnt>200)
                    {
                        circle_status++;
                        cnt%=200;
                    }
                    break;
                case 3:// step3 正常巡线
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;
                    if(adc_LL>circle_threshold)
                    {
                        circle_status++;
                    }
                    break;
                case 4:// step4 出环（正常巡线应该可以）
                    CURRENT_STATUS = Status_Stop;
                    pwm_set_duty(SERVO_PIN, SERVO_MOTOR_DUTY(82));
                    if(++cnt>200)// && adc_LL<circle_threshold
                    {
                        circle_status++;
                        cnt%=200;
                    }
                    break;
                case 5:// step5 出环后
//                    servo_control(Status_Common);
                    CURRENT_STATUS = Status_Common;
                    if(++cnt>200 && adc_RR<circle_threshold)
                    {
                        cnt%=200;
                        circle_status = 0;
                        right_circle_flag = false;
                    }
                    break;
            }
        }
    }

}

void img_handler()
{
    ImageProcess();
    ips200_show();
}
