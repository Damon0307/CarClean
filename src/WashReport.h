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
#include "Timer.h"
 

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

   
    void InitDefInfo(const char* file_path);
 
 //处理冲洗抓拍摄像头数据
    void DealWashIPCData(const json &p_json, Response &res);
//处理绕道摄像头数据
    void DealDetourIPCData(const json &p_json, Response &res);
//处理车辆进场数据
    void DealCarInIPCData(const json &p_json, Response &res);

//处理左右两侧AIIPC 冲洗干净程度数据
    void Deal_L_AIIPCData(const json &p_json, Response &res);
    void Deal_R_AIIPCData(const json &p_json, Response &res);
    void DealSerialData();
    void StartReportingProcess();
    void SetPassJsonFunc(std::function<void(json)> func);
 
    void AlarmReport(int exceptionType); //需要加锁？
    // 其实是util
    std::string getTime(const std::string &format);
    static unsigned short do_crc_table(unsigned char *ptr, int len);
    static std::string time_to_string(time_t t);
    static std::string utc_to_string(long long utcSeconds);

    int GetScore(float p);

private:
    int ai_detect_time;
    bool has_report;
    bool has_triger;
    int  wash_alarm_time;
//下面三个是固定信息存在配置文件中
    std::string deviceNo;
    std::string nvr_channel;
    std::string nvr_serial_num;
  
    std::mutex  sensor_data_mutex;       // 配合他的mutex

    json ResponseToIPC(int logic_type);

    // 构建符合云端协议的json数据
    json GetCaptureJson();
    json GetDeviceStatusJson();
    json GetCarInJson();  //进场时候上传给王工后台的消息
   
    bool GetAIIPCDetectResult();
    void ResetAllSensor();
    std::function<void(json)> PostJsonToServer;
   

 
    // 嵌套类 摄像头的抽象
    class IPC
    {
    public:
        IPC(/* args */){};
        ~IPC(){};
        bool has_trigger;
        bool working; //标志检测周期是否开始
        json json_data;
        void ResetStatus()
        {
            has_trigger=false;
            working =false;
            json_data={""};
        }
    };
 
    IPC ipc;
    AIIPC l_ai_ipc;
    AIIPC r_ai_ipc;
 
    //两个重要的时间
    //B点触发下降的时间，  用作AI摄像机的超时使用
    time_t   b_exit_time;
   
    int GetDirByCompareTime(const Point &a, const Point &b); // 通过比较两个点的先后时间得到方向

    int GetDirByIPC(int ipc_dir); // 通过IPC 

    void NotificationsToUart(int event_num); //发送事件信息给串口方便其控制NVR

//2分钟一次心跳
    Timer mHeartBearTimer;
 
    void StartHeartBeat();
//闸机的控制
   bool mBarrierGateNeed;
 

 
};

#endif // __WASHREPORT_H__