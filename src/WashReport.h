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
#include "AIIPCManager.h"
#include "uart.h"
#include "Timer.h"
#include "BarrierGate.h"
#include "Utils.h"

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

using json = nlohmann::json;
using namespace httplib;

using alarm_func_t = std::function<void(int)>;

using dl_report_wash_func_t = std::function<void(const json&,bool)>;
using dl_report_car_pass_func_t = std::function<void(const json&,bool)>;
using dl_report_status_func_t = std::function<void(const std::string&,int)>;

class WashReport
{

public:
    WashReport(/* args */);
    ~WashReport();

    void InitSerialComm(const char* file_path);
    void InitDefInfo(const char* file_path);

    // 处理冲洗抓拍摄像头数据
    void DealWashIPCData(const json &p_json, Response &res);
    // 处理绕道摄像头数据
    void DealDetourIPCData(const json &p_json, Response &res);
    // 处理车辆进场数据
    void DealCarInIPCData(const json &p_json, Response &res);

    // AI IPC 数据处理（当前启用: 左轮/右轮/车尾）
    void Deal_L_AIIPCData(const json &p_json, Response &res);
    void Deal_R_AIIPCData(const json &p_json, Response &res);
    void Deal_Tail_AIIPCData(const json &p_json, Response &res);

    // 暂时屏蔽: 车身两侧AI
    // void Deal_Side_L_AIIPCData(const json &p_json, Response &res);
    // void Deal_Side_R_AIIPCData(const json &p_json, Response &res);

    void DealSerialData();
    void StartReportingProcess();
    void SetPassJsonFunc(std::function<bool(json)> func);

#ifdef WASH_TEST_MODE
    // 测试模式：直接注入串口协议帧
    void InjectSerialFrame(const std::deque<char> &frame);
    // 测试模式：获取传感器内部状态
    json GetSensorStatusJson();
#endif

    void SetDLWashFunc(dl_report_wash_func_t func);
    void SetDLCarPassFunc(dl_report_car_pass_func_t func);
    void SetDLStatusFunc(dl_report_status_func_t func);

    void AlarmReport(int exceptionType);

    // 电源类型上报
    void ReportPowerType();

    // 工具函数委托（delegate to Utils.h）
    static std::string getTime(const std::string &format) { return ::getTime(format); }
    static std::string time_to_string(time_t t) { return ::time_to_string(t); }
    static std::string utc_to_string(long long utcSeconds) { return ::utc_to_string(utcSeconds); }
    static unsigned short do_crc_table(unsigned char *ptr, int len) { return ::do_crc_table(ptr, len); }
    static int GetScore(float p) { return ::GetScore(p); }

private:
    bool has_barrier_gate;
    bool has_report;
    bool has_triger;
    int  wash_alarm_time;

    std::string deviceNo;
    std::string nvr_channel;
    std::string nvr_serial_num;

    int power_type_report_interval = 10;
    Timer power_type_report_timer;
    int cur_power_type = 1;

    int serial_fd;
    std::string port_name;
    std::deque<char> serial_data_queue;
    std::mutex sensor_data_mutex;

    json ResponseToIPC(int logic_type);

    json GetCaptureJson();
    json GetDeviceStatusJson();
    json GetCarInJson();

    bool GetAIIPCDetectResult();
    void ResetAllSensor();
    std::function<bool(json)> PostJsonToServer;
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
        Timer alarm_timer;

        time_t begin_time;
        time_t finish_time;
        bool is_working;
        void DealStatus(char status)
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
                        time(&finish_time);
                        g_console_logger->debug("Water Pump finish time {}",time_to_string(finish_time));
                        g_file_logger->debug("Water Pump finish time {}",time_to_string(finish_time));
                    }
                }
            }
            else if (is_working == false)
            {
                if (status == 0x01)
                {
                    is_working = true;
                    time(&begin_time);

                    g_console_logger->debug("Water Pump start time {}",time_to_string(begin_time));
                    g_file_logger->debug("Water Pump start time {}",time_to_string(begin_time));

                    alarm_timer.setTimeout([&](){
                            alarm_func(2); //水泵的告警ID是2
                    },600*1000);
                }
                else
                {
                    ResetStatus();
                }
            }
        }
        bool IsEnoughTime()
        {
            if ((finish_time - begin_time) > 30)
            {
                return true;
            }
            return false;
        }
        void ResetStatus()
        {
            is_working = false;
            begin_time = 0;
            finish_time = 0;
            alarm_timer.stop();
        }
    };

    time_t car_active_time;

    Point point_b;
    WaterPump water_pump;
    IPC ipc;

    // AI IPC管理器（当前启用: 左轮/右轮/车尾）
    AIIPCManager ai_ipc_mgr;

    int ai_deal_delay_time = 3;
    time_t b_exit_time;

    int GetAlarmByWaterPump();
    int GetDirByIPC(int ipc_dir);

    void NotificationsToUart(int event_num);

    // 心跳
    Timer mHeartBearTimer;
    Timer mDlReportStatusTimer;
    void StartHeartBeat();

    // 闸机控制
    bool mBarrierGateNeed;
    BarrierGate* mBarrierGate;
    int mDelayTimeMs;
    int mKeepTimeMs;

    Timer mDelayTimer;
    Timer mKeepTimer;
};

#endif // __WASHREPORT_H__
