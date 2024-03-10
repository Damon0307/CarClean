#ifndef __WASHREPORT_H__
#define __WASHREPORT_H__

#include <string>
#include <functional>
#include <ctime>
#include <chrono>
#include <deque>
#include <mutex>
#include <time.h>
#include "spdlog/spdlog.h"
#include "json.hpp"
#include "httplib.h"
#include "Point.h"
#include "AIIPC.h"
#include "uart.h"
#include "Timer.h"

using json = nlohmann::json;
using namespace httplib;

using alarm_func_t = std::function<void(int)>;

using dl_report_wash_func_t = std::function<void(const json&,bool)>; 
using dl_report_car_pass_func_t = std::function<void(const json&)>;
using dl_report_status_func_t = std::function<void(const std::string&,int)>;   

class WashReport
{

public:
    WashReport(/* args */);
    ~WashReport();

    void InitSerialComm(const char* file_path);
    void InitDefInfo(const char* file_path);
 
 //处理冲洗抓拍摄像头数据
    void DealWashIPCData(const json &p_json, Response &res);
//处理绕道摄像头数据
    void DealDetourIPCData(const json &p_json, Response &res);

//处理左右两侧AIIPC 冲洗干净程度数据
    void Deal_L_AIIPCData(const json &p_json, Response &res);
    void Deal_R_AIIPCData(const json &p_json, Response &res);
    void DealSerialData();
    void StartReportingProcess();
    void SetPassJsonFunc(std::function<void(json)> func);

    void SetDLWashFunc(dl_report_wash_func_t func);
    void SetDLCarPassFunc(dl_report_car_pass_func_t func);  
    void SetDLStatusFunc(dl_report_status_func_t func);

    void AlarmReport(int exceptionType); //需要加锁？
    // 其实是util
    std::string getTime(const std::string &format);
    static unsigned short do_crc_table(unsigned char *ptr, int len);
    static std::string time_to_string(time_t t);
    static std::string utc_to_string(long long utcSeconds);

    int GetScore(float p);

private:
 
    bool has_report;
    bool has_triger;
    int  wash_alarm_time;
//下面三个是固定信息存在配置文件中
    std::string deviceNo;
    std::string nvr_channel;
    std::string nvr_serial_num;

    int serial_fd;
    std::string port_name;
    std::deque<char> serial_data_queue; // 目前看只需要保存一帧数据即可
    std::mutex  sensor_data_mutex;       // 配合他的mutex

    json ResponseToIPC(int logic_type);

    // 构建符合云端协议的json数据
    json GetCaptureJson();
    json GetDeviceStatusJson();
   
    bool GetAIIPCDetectResult();
    void ResetAllSensor();
    std::function<void(json)> PostJsonToServer;
    dl_report_wash_func_t dl_report_wash;   
    dl_report_car_pass_func_t dl_report_car_pass;   
    dl_report_status_func_t dl_report_status;

 
    // 嵌套类 摄像头的抽象
    class IPC
    {
    public:
        IPC(/* args */){};
        ~IPC(){};
        bool has_trigger;
        json json_data;
        void ResetStatus()
        {
            has_trigger=false;
            json_data={""};
        }
    };
    // 嵌套类，冲洗水泵的抽象
    class WaterPump
    {
    public:
        WaterPump(){
            alarm_timer.stop();
        };
        ~WaterPump(){};
         alarm_func_t alarm_func;
        //工作超时报警定时器
        Timer alarm_timer;

        time_t begin_time;
        time_t finish_time;
        bool is_working;
        void DealStatus(char status) // 处理开关量的状态
        {
            if (is_working == true)
            {
                if (status == 0x01)
                {
                    // 水泵仍在工作
                }
                else
                {
                    if (finish_time == 0)
                    {
                        time(&finish_time); // 水泵停止工作记录停止时间等待上报,
                        // 且只有被 ResetStatus 以后才会更新最后的停止时间
                        // 防止时间漂移
                    }
                }
            }
            else if (is_working == false)
            {
                if (status == 0x01)
                {
                    is_working = true;
                    time(&begin_time); // 流程开始，记录开始时间
                    alarm_timer.setTimeout([&](){
                            alarm_func(2); //水泵的告警ID是2
                    },600*1000);
                }
                else
                {
                    ResetStatus(); // 未工作，且无信号
                }
            }
        }
        bool IsEnoughTime() // 水泵的工作时间是否足够
        {
            if ((finish_time - begin_time) > 30)
            {
                return true;
            }
            return false;
        }
        void ResetStatus() // 重置工作状态
        {
            is_working = false;
            begin_time = 0;
            finish_time = 0;
        }
    };
    // a b点位置的光电模块 ，水泵,两侧AI摄像机
    Point point_a;
    Point point_b;
    WaterPump water_pump;
    IPC ipc;
    AIIPC l_ai_ipc;
    AIIPC r_ai_ipc;
 
    //两个重要的时间
    //B点触发下降的时间，  用作AI摄像机的超时使用
    time_t   b_exit_time;
  
    int GetAlarmTypeByPoint();//由于业务原因，需要改成由AB点的信号来确定水泵的冲洗时间够不够
    int GetDirByCompareTime(const Point &a, const Point &b); // 通过比较两个点的先后时间得到方向

     int GetDirByIPC(int ipc_dir); // 通过IPC 

    void NotificationsToUart(int event_num); //发送事件信息给串口方便其控制NVR

//2分钟一次心跳
    Timer mHeartBearTimer;
    Timer mDlReportStatusTimer;
    void StartHeartBeat();
 
};

#endif // __WASHREPORT_H__