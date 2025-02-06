#ifndef __POINT_H__
#define __POINT_H__

#include <time.h>
#include <stdio.h>
#include "Timer.h"
#include <string>
// 光电模块，由于业务的问题

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;


using alarm_func_t = std::function<void(int)>;

#define    ALARM_TIMEOUT            (600*1000)

class Point
{
private:
  //小模块 就不做数据保护了
 
public:
    Point(/* args */)
    {
        trigger_time = 0;
        cur_status = 0;
        is_working = false;
        is_exit = false;
        exit_car_leaving = false;
        alarm_timer.stop();
    };
    ~Point(){};
    alarm_func_t alarm_func;
    void SetPonintExit(bool status) // 设置该点是否为出口点
    {
        is_exit = status;
    }
    std::string  point_id;
    time_t trigger_time; // 光电模块触发时间
    time_t leave_time; // 出口光电模块下降沿触发时间 
    char cur_status;        // 当前光电模块状态
    bool is_working;    // 光电模块此次已经触发,需要整个流程结束才能被重置
    bool is_exit;           // 表明该点是否为出口
    bool exit_car_leaving;  // 出口点是否有车离开
 
    
    Timer alarm_timer; //雷达异常告警

    bool IsLeaving()       // 如果该点是出口则 下降沿有效
    {
        return exit_car_leaving;
    }
    void DealStatus(char status) // 处理开关量的状态
    {
        if (status == 0x01)
        {
            if (is_working == false)
            {
                 //异常报警开启
                alarm_timer.setTimeout([&](){
                    if(is_exit==true){
                    alarm_func(3);
                    }else{
                     alarm_func(1);
                    }
                },ALARM_TIMEOUT);
                is_working = true; // 处于工作流程中，上报结束以后才会被恢复
                cur_status = status;
                time(&trigger_time);
                g_console_logger->debug("Point {} Triggered",point_id);
                g_file_logger->debug("Point {} Triggered",point_id);
            }
            else
            {
            } // 已经触发过了，说明当前流程未结束，则忽略
        }
        else if (status == 0x00)
        {
            // 如果状态为0则分为以下三种情况
            // 1: 是否处于活动的流程状态
            // 2: 是否是出口

            if (is_working == false) // 不再工作状态，随意更新
            {
                cur_status = status;
                trigger_time = 0;
            }
            else // 处于流程中，且有一点信号消失
            {
                if (is_exit == false)
                {
                    // 非出口 不做更新处理
                }
                else
                { // 是出口，且被触发过
                    if (cur_status != status)
                    {
                        exit_car_leaving = true;
                        time(&leave_time);
                        g_console_logger->debug("Point B Exit Car Leaving time {}",leave_time);
                        
                        cur_status = status; // 更新状态
                        // 表明出口有下降沿信号
                    }

                } // 在工作状态，即使 A点检测不到状态也不进行更新
            }
        }
    }
    void SetPointID(const std::string& pid)
    {
        point_id = pid;
    }

    void ResetStatus() // 此次流程上报完成 重置模块
    {
        cur_status = 0;
        trigger_time = 0;
        leave_time = 0;
        is_working = false;
        exit_car_leaving =false;
        alarm_timer.stop();
    }
};

#endif // __POINT_H__