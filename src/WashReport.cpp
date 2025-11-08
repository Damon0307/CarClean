#include "WashReport.h"
#include <future>

// 直连功能是否启用
#define DIRECTOR_LINK_ENABLE 0

#define NORMAL_REPLY_TO_IPC 1
// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

// uart msg define
const unsigned char event_normal[] = {0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0xde, 0x31, 0xaa};
const unsigned char event_dirty[] = {0x55, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe2, 0x0d, 0xaa};

const char *time_format = "%Y-%m-%d %H:%M:%S";
/* CRC余式表 */
const unsigned int crc_table[256] = {
    0x0000,
    0xc0c1,
    0xc181,
    0x0140,
    0xc301,
    0x03c0,
    0x0280,
    0xc241,
    0xc601,
    0x06c0,
    0x0780,
    0xc741,
    0x0500,
    0xc5c1,
    0xc481,
    0x0440,
    0xcc01,
    0x0cc0,
    0x0d80,
    0xcd41,
    0x0f00,
    0xcfc1,
    0xce81,
    0x0e40,
    0x0a00,
    0xcac1,
    0xcb81,
    0x0b40,
    0xc901,
    0x09c0,
    0x0880,
    0xc841,
    0xd801,
    0x18c0,
    0x1980,
    0xd941,
    0x1b00,
    0xdbc1,
    0xda81,
    0x1a40,
    0x1e00,
    0xdec1,
    0xdf81,
    0x1f40,
    0xdd01,
    0x1dc0,
    0x1c80,
    0xdc41,
    0x1400,
    0xd4c1,
    0xd581,
    0x1540,
    0xd701,
    0x17c0,
    0x1680,
    0xd641,
    0xd201,
    0x12c0,
    0x1380,
    0xd341,
    0x1100,
    0xd1c1,
    0xd081,
    0x1040,
    0xf001,
    0x30c0,
    0x3180,
    0xf141,
    0x3300,
    0xf3c1,
    0xf281,
    0x3240,
    0x3600,
    0xf6c1,
    0xf781,
    0x3740,
    0xf501,
    0x35c0,
    0x3480,
    0xf441,
    0x3c00,
    0xfcc1,
    0xfd81,
    0x3d40,
    0xff01,
    0x3fc0,
    0x3e80,
    0xfe41,
    0xfa01,
    0x3ac0,
    0x3b80,
    0xfb41,
    0x3900,
    0xf9c1,
    0xf881,
    0x3840,
    0x2800,
    0xe8c1,
    0xe981,
    0x2940,
    0xeb01,
    0x2bc0,
    0x2a80,
    0xea41,
    0xee01,
    0x2ec0,
    0x2f80,
    0xef41,
    0x2d00,
    0xedc1,
    0xec81,
    0x2c40,
    0xe401,
    0x24c0,
    0x2580,
    0xe541,
    0x2700,
    0xe7c1,
    0xe681,
    0x2640,
    0x2200,
    0xe2c1,
    0xe381,
    0x2340,
    0xe101,
    0x21c0,
    0x2080,
    0xe041,
    0xa001,
    0x60c0,
    0x6180,
    0xa141,
    0x6300,
    0xa3c1,
    0xa281,
    0x6240,
    0x6600,
    0xa6c1,
    0xa781,
    0x6740,
    0xa501,
    0x65c0,
    0x6480,
    0xa441,
    0x6c00,
    0xacc1,
    0xad81,
    0x6d40,
    0xaf01,
    0x6fc0,
    0x6e80,
    0xae41,
    0xaa01,
    0x6ac0,
    0x6b80,
    0xab41,
    0x6900,
    0xa9c1,
    0xa881,
    0x6840,
    0x7800,
    0xb8c1,
    0xb981,
    0x7940,
    0xbb01,
    0x7bc0,
    0x7a80,
    0xba41,
    0xbe01,
    0x7ec0,
    0x7f80,
    0xbf41,
    0x7d00,
    0xbdc1,
    0xbc81,
    0x7c40,
    0xb401,
    0x74c0,
    0x7580,
    0xb541,
    0x7700,
    0xb7c1,
    0xb681,
    0x7640,
    0x7200,
    0xb2c1,
    0xb381,
    0x7340,
    0xb101,
    0x71c0,
    0x7080,
    0xb041,
    0x5000,
    0x90c1,
    0x9181,
    0x5140,
    0x9301,
    0x53c0,
    0x5280,
    0x9241,
    0x9601,
    0x56c0,
    0x5780,
    0x9741,
    0x5500,
    0x95c1,
    0x9481,
    0x5440,
    0x9c01,
    0x5cc0,
    0x5d80,
    0x9d41,
    0x5f00,
    0x9fc1,
    0x9e81,
    0x5e40,
    0x5a00,
    0x9ac1,
    0x9b81,
    0x5b40,
    0x9901,
    0x59c0,
    0x5880,
    0x9841,
    0x8801,
    0x48c0,
    0x4980,
    0x8941,
    0x4b00,
    0x8bc1,
    0x8a81,
    0x4a40,
    0x4e00,
    0x8ec1,
    0x8f81,
    0x4f40,
    0x8d01,
    0x4dc0,
    0x4c80,
    0x8c41,
    0x4400,
    0x84c1,
    0x8581,
    0x4540,
    0x8701,
    0x47c0,
    0x4680,
    0x8641,
    0x8201,
    0x42c0,
    0x4380,
    0x8341,
    0x4100,
    0x81c1,
    0x8081,
    0x4040,
};
/***********字段处理 ICP - SERVER**************/
int CarColorConvert(int p)
{
    switch (p)
    {
    case 0:
        return 3;
    case 1:
        return 1;
    case 2:
        return 2;
    case 6:
        return 1;
    case 8:
        return 4;
    case 5:
        return 5;
    default:
        break;
    }
    return 0;
}

int CarTypeConvert(int p)
{
    switch (p)
    {
    case 0x05:
    case 0x07:
        return 2;
    }
    return 1;
}

/*****************************/

WashReport::WashReport(/* args */)
{
    has_barrier_gate = false;
    point_b.ResetStatus();
    point_b.SetPonintExit(true); // 设置B点为出口点
    point_b.SetPointID("B");
    water_pump.ResetStatus();
    ipc.ResetStatus();
    serial_data_queue.clear();
    // need lock?
    point_b.alarm_func = std::bind(&WashReport::AlarmReport, this, std::placeholders::_1);
    water_pump.alarm_func = std::bind(&WashReport::AlarmReport, this, std::placeholders::_1);
}

WashReport::~WashReport()
{
    if (NULL != mBarrierGate)
    {
        delete mBarrierGate;
        mBarrierGate = NULL;
    }
}

void WashReport::InitSerialComm(const char *file_path)
{
    serial_fd = -1;
    port_name = "/dev/ttyS3";
    serial_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (serial_fd == -1)
    {
        perror("Failed to open serial port");
    }
    // 进行115200 8 0 1 N 的串口设置
    struct termios options;
    // 定义 BAUDRATE

    tcgetattr(serial_fd, &options);
    bzero(&options, sizeof(options));
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(serial_fd, TCIFLUSH);
    tcsetattr(serial_fd, TCSANOW, &options);
    cfsetospeed(&options, B115200);
    cfsetispeed(&options, B115200);
    tcsetattr(serial_fd, TCSANOW, &options);

    sleep(1);
}

void WashReport::InitDefInfo(const char *file_path)
{

    std::ifstream f(file_path);
    json data = json::parse(f);

    deviceNo = data["deviceNo"];
    nvr_channel = data["nvr_channel"];
    nvr_serial_num = data["nvr_serial_num"];
    wash_alarm_time = data["wash_alarm_time"];

    // 判断一下json中存不存在 BarrierGate字段，如果有是不是true
    if (data.contains("BarrierGate") && data["BarrierGate"])
    {
        mBarrierGate = new BarrierGate();
        has_barrier_gate = true;
        // 初始化保持时间和延迟时间

        int p_delay_time = data["delay_time"];
        int p_keep_time = data["keep_time"];

        this->mDelayTimeMs = p_delay_time * 1000;
        this->mKeepTimeMs = p_keep_time * 1000;
    }
    else
    {
        mBarrierGate = NULL;
    }

    // 判断存不存在 time_after_b 字段，如果存在赋值给ai_deal_delay_time
    if (data.contains("time_after_b"))
    {
        ai_deal_delay_time = data["time_after_b"];
    }

    g_console_logger->debug("wash alarm time set to {}", wash_alarm_time);

    f.close();
}

// 接收到摄像头推送的抓拍数据
void WashReport::DealWashIPCData(const json &p_json, Response &res)
{

    //  std::cout << p_json.dump() << std::endl;
    ipc.json_data = p_json;

    json response = ResponseToIPC(NORMAL_REPLY_TO_IPC);
    res.set_content(response.dump(), "application/json");
    // 主动清除B点工作状态
    point_b.is_working = false;

    if (p_json.contains("AlarmInfoPlate") && p_json["AlarmInfoPlate"].contains("result") && p_json["AlarmInfoPlate"]["result"].contains("PlateResult") && p_json["AlarmInfoPlate"]["result"]["PlateResult"].contains("license"))
    {
        g_file_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());
        g_console_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());
        // 清空左右两侧AI摄像头的数据
        r_ai_ipc.ResetStatus();
        l_ai_ipc.ResetStatus();

        // 清除B点的定时器
        point_b.ResetStatus();
        // 清除水泵的定时器
        water_pump.ResetStatus();

        g_console_logger->debug("Both sides AI IPC status  and point b   water pump have been reset");
        g_file_logger->debug("Both sides AI IPC status  and point b water pump have been reset");
        time(&car_active_time); // 流程开始，记录开始时间
        std::cout << "Got Car license : " << p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump() << std::endl;
    }
    else
    {
        g_file_logger->debug("Got Wash IPC Data  NO  Licenses!!! ");
        g_console_logger->debug("Got Wash IPC Data NO  Licenses!!!  ");
    }
    ipc.has_trigger = true;
}

// todo 接收到绕道摄像头推送的数据
void WashReport::DealDetourIPCData(const json &p_json, Response &res)
{
    // 组符合后端服务器的JSON
    json capture_res = GetCaptureJson(); // 已经包含默认信息
    capture_res["captureTime"] = utc_to_string(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
    capture_res["ztcCph"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
    capture_res["ztcColor"] = CarColorConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
    capture_res["vehicleType"] = CarTypeConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
    capture_res["picture"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];
    capture_res["enterTime"] = "";
    capture_res["leaveTime"] = "";

    // 绕道也许没有传感器所以直接填写告警编码号
    capture_res["alarmType"] = 1;
    capture_res["frontWheelWashTime"] = 0;
    capture_res["hindWheelWashTime"] = 0;

    capture_res.erase("rightclean");
    capture_res.erase("leftclean");

    int ipc_dir = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];

    // 只有明确出场上报，左右摇摆的都不上报为绕道
    if (ipc_dir == 4) // 由远及近，对应车牌轨迹从上到下，方向是向下
    {
        capture_res["direction"] = 1; // 0 进场 1出场
        PostJsonToServer(capture_res);
        // printf("Report Detour %s    time %s \n",capture_res["ztcCph"].dump().c_str(),capture_res["captureTime"].dump().c_str()); // 输出车牌
        g_console_logger->debug("Report Detour {} ", capture_res["ztcCph"].dump().c_str());
        g_file_logger->debug("Report Detour {} ", capture_res["ztcCph"].dump().c_str());
#if (DIRECTORY_REPORT_ENABLE == 1)
        dl_report_wash(capture_res, true);
#endif
    }
    else
    {
        g_console_logger->warn("Not Report Detour with direction {} of  {}", ipc_dir, capture_res["ztcCph"].dump().c_str());
        g_file_logger->warn("Not Report Detour with direction {} of  {}", ipc_dir, capture_res["ztcCph"].dump().c_str());
        // printf("Not Report Detour with direction %d \n",ipc_dir);
    }

    // ResetAllSensor(); 绕道未触发传感器
    json response = ResponseToIPC(NORMAL_REPLY_TO_IPC);
    res.set_content(response.dump(), "application/json");
}

void WashReport::DealCarInIPCData(const json &p_json, Response &res)
{
    json car_in_json = GetCarInJson();
    car_in_json["captureTime"] = utc_to_string(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
    car_in_json["ztcCph"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
    car_in_json["ztcColor"] = CarColorConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
    car_in_json["vehicleType"] = CarTypeConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
    car_in_json["picture"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];
    car_in_json["direction"] = 0;

    PostJsonToServer(car_in_json);
#if (DIRECTOR_LINK_ENABLE == 1)
    dl_report_car_pass(car_in_json, true);
#endif

    g_console_logger->debug("Report Car in {} ", car_in_json["ztcCph"].dump().c_str());
    g_file_logger->debug("Report Car in {} ", car_in_json["ztcCph"].dump().c_str());

    json response = ResponseToIPC(NORMAL_REPLY_TO_IPC);
    res.set_content(response.dump(), "application/json");
}

// 处理两侧车轮冲洗干净程度的数据
void WashReport::Deal_L_AIIPCData(const json &p_json, Response &res)
{
    if (p_json.contains("label"))
    {
        if (point_b.is_working)
        {
            l_ai_ipc.DealAIIPCData(p_json);
            g_console_logger->debug("Deal_L_AIIPCData clean res {}  ", p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_L_AIIPCData clean res {} ", p_json["label"].dump().c_str());
        }
        else
        {
            // 如果当前时间与 point_b的leave_time相差  ai_deal_delay_time 以内则处理
            time_t cur_time;
            time(&cur_time);
            if (difftime(cur_time, point_b.leave_time) <= ai_deal_delay_time)
            {
                l_ai_ipc.DealAIIPCData(p_json);
                g_console_logger->debug("Deal_L_AIIPCData clean res  in delay time {} ", p_json["label"].dump().c_str());
                g_file_logger->debug("Deal_L_AIIPCData clean res  in delay time {} ", p_json["label"].dump().c_str());
            }
            else
            {
                g_console_logger->debug("Rejected handle Left AIIPC cause no point b working");
                g_file_logger->debug("Rejected handle Left AIIPC cause no point b working");
            }
        }
    }

    res.set_content("OK", "text/plain");
}
void WashReport::Deal_R_AIIPCData(const json &p_json, Response &res)
{

    if (p_json.contains("label"))
    {
        if (point_b.is_working)
        {
            r_ai_ipc.DealAIIPCData(p_json);
            g_console_logger->debug("Deal_R_AIIPCData clean res {} ", p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_R_AIIPCData clean res {}", p_json["label"].dump().c_str());
        }
        else
        {

            // 如果当前时间与 point_b的leave_time相差  ai_deal_delay_time 以内则处理
            time_t cur_time;
            time(&cur_time);
            if (difftime(cur_time, point_b.leave_time) <= ai_deal_delay_time)
            {
                r_ai_ipc.DealAIIPCData(p_json);
                g_console_logger->debug("Deal_R_AIIPCData clean res  in delay time {} ", p_json["label"].dump().c_str());
                g_file_logger->debug("Deal_R_AIIPCData clean res  in delay time {} ", p_json["label"].dump().c_str());
            }
            else
            {
                g_console_logger->debug("Rejected handle Right AIIPC cause no point b working");
                g_file_logger->debug("Rejected handle Right AIIPC cause no point b working");
            }
        }
    }

    res.set_content("OK", "text/plain");
}

// 处理串口数据
// 帧头	开关量状态	CRC16	帧尾
// 0x55	5位二进制数	2字节CRC16校验值	0xAA
void WashReport::DealSerialData()
{

    static int pa_prev_value = -1;
    static int pb_prev_value = -1;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(serial_fd, &readfds);

    int ret = select(serial_fd + 1, &readfds, NULL, NULL, NULL);

    if (ret < 0)
    {
        // 错误处理
    }
    else if (ret == 0)
    {
        // 超时,这里不会发生
    }
    else
    {
        // 有数据可读
        if (FD_ISSET(serial_fd, &readfds))
        {

            // 读取数据
            uint8_t buf[128] = {0};
            int buf_len = read(serial_fd, buf, sizeof(buf));

            if (buf_len > 0)
            {
                for (int i = 0; i < buf_len; i++)
                {
                    serial_data_queue.push_back(buf[i]);
                }

                if (serial_data_queue.size() >= 9) // 存在一包完整的数据
                {
                    bool have_decode = false;

                    for (int i = 0; i < serial_data_queue.size(); i++)
                    {
                        if (serial_data_queue[i] == 0x55 && (i + 8) <= (serial_data_queue.size() - 1))
                        { // 寻找到帧头，且后续长度足够解
//把每个字节的数据在一行中打印出来
                            // for (int j = 0; j < 9; j++)
                            // {
                            //     printf("0x%02X ", (unsigned char)serial_data_queue[i + j]);
                            // }
                            // printf("\n");

                            have_decode = true;
                            // 校验CRC16
                            // point_a.DealStatus(serial_data_queue[i + 1]);
                            point_b.DealStatus(serial_data_queue[i + 2]);
                            water_pump.DealStatus(serial_data_queue[i + 5]);
                            break;
                        }
                    }
                    std::deque<char> zero;
                    serial_data_queue.swap(zero);
                }
            }
            else
            {
                // 无数据
            }
        }
    }
}

void WashReport::SetPassJsonFunc(std::function<bool(json)> func)
{
    PostJsonToServer = func;
}

json WashReport::GetCaptureJson()
{
    json res;
    res["xmbh"] = "XMBH00000003";
    res["deviceNo"] = deviceNo;
    res["captureTime"] = "";
    res["ztcCph"] = "";
    res["ztcColor"]= "";
    res["vehicleType"];
    res["enterTime"] = "";
    res["leaveTime"] = "";
    res["alarmType"]=5; //告警类型 1：车辆绕行  2：冲洗时间不足  3：未冲洗  4：其他  5：正常冲洗
 
    res["frontWheelWashTime"] = 0;
    res["hindWheelWashTime"] = 0;
    res["deviceSerial"] = nvr_serial_num;
    res["localIndex"] = nvr_channel;
    res["picture"] = " ";
    res["dataType"] = 1;
    res["direction"];
    res["cleanRes"] = 0;          // 车辆车轮清洗结果 1：未知  2：冲洗干净  3：未冲洗干净
    res["leftphotoUrl"] = ""; // 车辆左侧抓拍图片
    res["rightphotoUrl"] = "";
    res["rightclean"] = 0;
    res["leftclean"] = 0;   // 车辆左侧冲洗洁净 度数值
    res["gate_status"] = 0; // 道闸状态 0 未配置道闸， 1 正常， 2 脏车拒绝开闸。
    res["open_time"] = "";  // 开闸时间 修复: 使用赋值
    return res;
}
json WashReport::GetDeviceStatusJson()
{
    json res;
    res["deviceNo"] = deviceNo;
    res["updateTime"];
    res["status"];
    res["dataType"] = 2;
    return res;
}

json WashReport::GetCarInJson()
{
    json res;
    res["xmbh"] = "XMBH00000003";
    res["deviceNo"] = deviceNo;
    res["captureTime"] = "";
    res["ztcCph"] = "";
    res["ztcColor"];
    res["vehicleType"];
    res["picture"] = "";
    res["dataType"] = 4;
    res["direction"] = 0;

    return res;
}

std::string WashReport::getTime(const std::string &format)
{
    std::time_t now = std::time(nullptr);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), format.c_str(), std::localtime(&now));
    return std::string(buffer);
}

// 获取AI摄像机的检测结果 同步获取
bool WashReport::GetAIIPCDetectResult()
{
    bool l_res = l_ai_ipc.GetResult();
    bool r_res = r_ai_ipc.GetResult();
    if (l_res && r_res)
    {
        return true;
    }
    return false;
}

// 回复给IPC客户端的数据
json WashReport::ResponseToIPC(int logic_type)
{
    json res;
    switch (logic_type)
    {
    case NORMAL_REPLY_TO_IPC:
    {
        res["Response_AlarmInfoPlate"]["info"] = "ok";
        res["Response_AlarmInfoPlate"]["plateid"] = 123;
        res["Response_AlarmInfoPlate"]["channelNum"] = 0;
        // 如果回复OK 会一直触发截图推送
        // res["Response_AlarmInfoPlate"]["manualTrigger"] = "ok";
        // res["Response_AlarmInfoPlate"]["TriggerImage"]["port"] = 8080;
        // res["Response_AlarmInfoPlate"]["TriggerImage"]["snapImageRelativeUrl"] = "/";
        res["Response_AlarmInfoPlate"]["is_pay"] = true;
    }
    break;
    default:
        break;
    }
    return res;
}

// 查表法计算crc  https://blog.csdn.net/whik1194/article/details/108518336
unsigned short WashReport::do_crc_table(unsigned char *ptr, int len)
{
    unsigned short crc = 0xFFFF;

    while (len--)
    {
        crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
    }

    return (crc);
}

// 处理数据
void WashReport::StartReportingProcess()
{
    printf("WashReporter StartReportingProcess With Single Radar...\n");
    StartHeartBeat();

    bool last_point_b_status = true;
    bool last_point_b_working = true;
    bool exit_car_leaving = true;

    while (1)
    {
        DealSerialData();
        if (ipc.has_trigger == true)
        {  

            if (point_b.is_working != last_point_b_working || point_b.cur_status != last_point_b_status || point_b.exit_car_leaving != exit_car_leaving)
            {
                // printf("A working  ,A status %d  %d \n", point_a.is_working, point_a.cur_status);
                g_console_logger->debug("B Working  Status  Leaving {}  {}  {} ", static_cast<int>(point_b.is_working), static_cast<int>(point_b.cur_status), static_cast<int>(point_b.exit_car_leaving));
                last_point_b_working = point_b.is_working;
                last_point_b_status = point_b.cur_status;
                exit_car_leaving = point_b.exit_car_leaving;
            }
            if ((point_b.is_working) && (point_b.IsLeaving() == true)) // B点触发，且下降沿
            {
                // 组符合后端服务器的JSON
                json capture_res = GetCaptureJson(); // 已经包含默认信息
                capture_res["captureTime"] = utc_to_string(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
                capture_res["ztcCph"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
                capture_res["ztcColor"] = CarColorConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
                capture_res["vehicleType"] = CarTypeConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
                //*进入时间就是抓拍时间
                capture_res["enterTime"] = utc_to_string(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
               
                double diff_seconds = difftime(point_b.leave_time, car_active_time);
                long long time_interval = static_cast<long long>(diff_seconds); // 如果需要整数部分

                // 假设已经检查过JSON路径的有效性
                long long base_time = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"].get<long long>();
                long long final_leave_time = base_time + time_interval;

                capture_res["leaveTime"] = utc_to_string(final_leave_time);

                // 打印enterTime 和 leaveTime
                std::cout << "enter time: " << capture_res["enterTime"] << std::endl;
                std::cout << "leave time: " << capture_res["leaveTime"] << std::endl;
                //根据进入时间 和离开时间计算冲洗时间，以及水泵的状态得出alarmType
//!告警类型
// 1：车辆绕行
// 2：冲洗时间不足
// 3：未冲洗
// 4：其他
// 5：正常冲洗
               capture_res["alarmType"] =GetAlarmByWaterPump();

                g_console_logger->debug("leave time: {}", capture_res["leaveTime"].dump().c_str());
                g_file_logger->debug("leave time: {}", capture_res["leaveTime"].dump().c_str());
                //记录告警类型到日志
                if(capture_res["alarmType"] == 5)
                {
                 g_console_logger->debug("Alarm Type is 正常冲洗 for  {} ", capture_res["ztcCph"].dump().c_str());
                 g_file_logger->debug("Alarm Type is 正常冲洗 for  {} ", capture_res["ztcCph"].dump().c_str());
                }else if(capture_res["alarmType"] == 2)
                {
                 g_console_logger->debug("Alarm Type is 冲洗时间不足 for  {} ", capture_res["ztcCph"].dump().c_str());
                 g_file_logger->debug("Alarm Type is 冲洗时间不足 for  {} ", capture_res["ztcCph"].dump().c_str());
                }else if(capture_res["alarmType"] == 3)
                {
                 g_console_logger->debug("Alarm Type is 未冲洗 for  {} ", capture_res["ztcCph"].dump().c_str());
                 g_file_logger->debug("Alarm Type is 未冲洗 for  {} ", capture_res["ztcCph"].dump().c_str());
                }else if(capture_res["alarmType"] == 4)
                {
                 g_console_logger->debug("Alarm Type is 其他 for  {} ", capture_res["ztcCph"].dump().c_str());
                 g_file_logger->debug("Alarm Type is 其他 for  {} ", capture_res["ztcCph"].dump().c_str());
                }else if(capture_res["alarmType"] == 1)
                {
                 g_console_logger->debug("Alarm Type is 车辆绕行 for  {} ", capture_res["ztcCph"].dump().c_str());
                 g_file_logger->debug("Alarm Type is 车辆绕行 for  {} ", capture_res["ztcCph"].dump().c_str());
                }
              
                // 前后轮冲洗时间改为 0
                capture_res["frontWheelWashTime"] = 0;
                capture_res["hindWheelWashTime"] = 0;

                capture_res["picture"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];

                int ipc_dir = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];
                capture_res["direction"] = GetDirByIPC(ipc_dir); // 通过IPC

                // 细粒度时间窗口采集：在整个 ai_deal_delay_time 秒窗口内每200ms轮询状态，持续接收其他线程推送的数据，不提前退出
                bool ai_all_res = false;
                auto window_start = std::chrono::steady_clock::now();
                auto deadline = window_start + std::chrono::seconds(ai_deal_delay_time);

                g_console_logger->debug("clollecting the ai ipc data in  {} seconds window ", ai_deal_delay_time);
                g_file_logger->debug("clollecting the ai ipc data in  {} seconds window ", ai_deal_delay_time);

                while (std::chrono::steady_clock::now() < deadline)
                {
                    //打印剩余多少收集时间
                    auto remaining_time = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
                    g_console_logger->debug("remaining collect  time {} ms", remaining_time);
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                }

                ai_all_res = GetAIIPCDetectResult(); // 获取AI摄像机数据

                g_console_logger->debug("AI IPC data collect finished with result {} ", ai_all_res);
                g_file_logger->debug("AI IPC data collect finished with result {} ", ai_all_res);

                if (ai_all_res)
                {
                    //  获取AI摄像机数据
                    json l_detect_json_data = l_ai_ipc.GetDetectRes();
                    json r_detect_json_data = r_ai_ipc.GetDetectRes();

                    // 检查 "label" 字段是否存在，如果不存在则默认为 "unknown"
                    std::string l_label = l_detect_json_data.contains("label") ? l_detect_json_data["label"] : "unknown";
                    std::string r_label = r_detect_json_data.contains("label") ? r_detect_json_data["label"] : "unknown";

                    if (r_label == "clean" && l_label == "clean")
                    {
                        capture_res["cleanRes"] = 2;

                        g_console_logger->debug("All clean {}", capture_res["ztcCph"].dump().c_str());
                        g_file_logger->debug("All clean {}", capture_res["ztcCph"].dump().c_str());

                       // 闸机控制,异步操作根据延迟时间和保持时间控制BarrierGateCtrl
                        if (NULL != mBarrierGate)
                        {

                            g_console_logger->debug("Gate Will be open for {} in  {} ms ", capture_res["ztcCph"].dump().c_str(), mDelayTimeMs);
                            g_file_logger->debug("Gate Will be open for {} in  {} ms ", capture_res["ztcCph"].dump().c_str(), mDelayTimeMs);

                            mBarrierGate->BarrierGateCtrl(false);
                            mDelayTimer.setTimeout([this]()
                                                   { mBarrierGate->BarrierGateCtrl(true); }, mDelayTimeMs);

                            mKeepTimer.setTimeout([this]()
                                                  { mBarrierGate->BarrierGateCtrl(false); }, mDelayTimeMs + mKeepTimeMs);
                            // 实际上闸机的关闭是自己控制的现在
                        }
                        if (has_barrier_gate)
                        {
                            capture_res["gate_status"] = 1;
                            // 开闸时间是 当前时间+mDelayTimeMs
                            time_t now = time(nullptr);
                            time_t open_time = now + mDelayTimeMs / 1000;
                            capture_res["open_time"] = time_to_string(open_time);
                        }
                        else
                        {
                            capture_res["gate_status"] = 0;
                        }
                    }
                    else
                    {
                        capture_res["cleanRes"] = 3;
                        g_console_logger->debug("With dirty {}", capture_res["ztcCph"].dump().c_str());
                        g_file_logger->debug("With dirty  {}", capture_res["ztcCph"].dump().c_str());

                        if (has_barrier_gate)
                        {
                            capture_res["gate_status"] = 2;
                        }
                        else
                        {
                            capture_res["gate_status"] = 0;
                        }
                    }

                    // 检查 "img_base64" 字段是否存在，如果不存在则默认为 ""
                    std::string l_photo_url = l_detect_json_data.contains("img_base64") ? l_detect_json_data["img_base64"] : "";
                    std::string r_photo_url = r_detect_json_data.contains("img_base64") ? r_detect_json_data["img_base64"] : "";
                    capture_res["leftphotoUrl"] = l_photo_url; // 车辆左侧抓拍图片
                    capture_res["rightphotoUrl"] = r_photo_url;

                    // 检查 "score" 字段是否存在，如果不存在则默认为 0
                    float l_score = l_detect_json_data.contains("score") ? l_detect_json_data["score"].get<float>() : 0.0;
                    float r_score = r_detect_json_data.contains("score") ? r_detect_json_data["score"].get<float>() : 0.0;
                    capture_res["leftclean"] = l_score; // 车辆左侧冲洗洁净 度数值
                    capture_res["rightclean"] = r_score;
                }
                else
                {
                    capture_res["cleanRes"] = 1;  // 超时，冲洗结果为未知
                    capture_res["leftclean"] = 0; // 车辆左侧冲洗洁净 度数值
                    capture_res["rightclean"] = 0;
                    // 超时处理  重置动作放在流程上报末尾统一触
                    g_console_logger->debug("Timeout occurred while waiting for ai ipc data");
                    g_file_logger->debug("Timeout occurred while waiting for ai ipc data");
                }

                bool post_res = PostJsonToServer(capture_res);

                if (post_res)
                {
                    g_console_logger->debug("Report Wash Capture Success  {} ", capture_res["ztcCph"].dump().c_str());
                    g_file_logger->debug("Report Wash Capture Success  {} ", capture_res["ztcCph"].dump().c_str());
                }
                else
                {
                    g_console_logger->debug("Report Wash Capture Failed  {} ", capture_res["ztcCph"].dump().c_str());
                    g_file_logger->debug("Report Wash Capture Failed  {} ", capture_res["ztcCph"].dump().c_str());
                }

                ResetAllSensor();

                std::cout << "===================Pass and reset===================" << std::endl;

                g_file_logger->debug("===================Pass and reset===================");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void WashReport::NotificationsToUart(int event_num)
{
    // 不干净 事件2
    // 其他 事件1
    if (event_num == 1)
    {
        UART_Send(serial_fd, (char *)event_normal, sizeof(event_normal));
    }
    else if (event_num == 2)
    {
        UART_Send(serial_fd, (char *)event_dirty, sizeof(event_dirty));
    }
}

//! 要么在这里加，要么在NTP服务时候加8小时

std::string WashReport::time_to_string(time_t t)
{
    // 线程安全版本：使用 localtime_r (或 gmtime_r) 而不是非线程安全的 localtime
    std::tm tm_buf;
    if (localtime_r(&t, &tm_buf) == nullptr)
    {
        g_console_logger->error("time_to_string localtime_r failed for {}", static_cast<long long>(t));
        return "";
    }
    char buf[20]; // YYYY-MM-DD HH:MM:SS => 19 + null
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf) == 0)
    {
        g_console_logger->error("time_to_string strftime failed");
        return "";
    }
    return std::string(buf);
}

std::string WashReport::utc_to_string(long long utcSeconds)
{
    // 直接加 8 小时偏移并格式化，不手动修改 tm_hour，避免跨日处理错误
    if (utcSeconds < 0)
    {
        g_console_logger->warn("utc_to_string received negative utcSeconds {}", utcSeconds);
    }
    std::time_t t = static_cast<std::time_t>(utcSeconds + 8LL * 60 * 60); // 转为北京时间 (UTC+8)
    std::tm tm_buf;
    if (gmtime_r(&t, &tm_buf) == nullptr) // gmtime_r 得到调整后 UTC+8 对应的结构体
    {
        g_console_logger->error("utc_to_string gmtime_r failed for {}", utcSeconds);
        return "";
    }
    char buf[20];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf) == 0)
    {
        g_console_logger->error("utc_to_string strftime failed");
        return "";
    }
    return std::string(buf);
}

void WashReport::ResetAllSensor()
{
    point_b.ResetStatus();
    water_pump.ResetStatus();
    ipc.ResetStatus();
    l_ai_ipc.ResetStatus();
    r_ai_ipc.ResetStatus();
}

int WashReport::GetAlarmByWaterPump()
{
    // 定义一个1970年的默认时间
    const time_t def_time = 0;

    if (water_pump.begin_time == 0) // 水泵未工作
    {
        return 3;
    }
    if (water_pump.begin_time != 0 && water_pump.finish_time == 0) // 水泵工作中
    {
        // 从B点离开时间减去水泵开始时间看看大不大于设定的冲洗时间
        if (difftime(point_b.leave_time, water_pump.begin_time) > wash_alarm_time)
        {
            return 5; //! 正常冲洗
        }
        else
        {
            return 2; //! 冲洗时间不够
        }
    }

    if (water_pump.begin_time != def_time && water_pump.finish_time != def_time) // 水泵工作结束
    {
        // 完全从水泵的finish_time 减去begin_time 大不大于wash_alarm_time
        if (difftime(water_pump.finish_time, water_pump.begin_time) > wash_alarm_time)
        {
            return 5; //! 正常冲洗
        }
        else
        {
            return 2; //! 冲洗时间不够
        }
    }
    return 4; // 其他
}

int WashReport::GetDirByIPC(int ipc_dir)
{
    if (ipc_dir == 4) // 由远及近，对应车牌轨迹从上到下，方向是向下
    {
        return 1;
    }
    else if (ipc_dir == 3)
    {
        return 0;
    }
    return 1;
}

void WashReport::AlarmReport(int exceptionType)
{
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::to_time_t(now);
    std::tm *time_info = std::localtime(&time_point);
    std::ostringstream oss;
    oss << std::put_time(time_info, "%Y-%m-%d %H:%M:%S");

    json res;
    res["deviceNo"] = deviceNo;
    res["alarmTime"] = oss.str(); // 告警时间（精确到秒）
    res["exceptionType"] = exceptionType;
    res["dataType"] = 3;
    PostJsonToServer(res);

    // 异常以后复位该模块 但不复位其他模块
    if (exceptionType == 1)
    {
        // 记录异常
        g_console_logger->debug("Point A alarm ");
        g_file_logger->debug("Point A alarm ");
    }
    else if (exceptionType == 2)
    {
        g_console_logger->debug("Water_pump  alarm ");
        g_file_logger->debug("Water_pump  alarm ");
        water_pump.ResetStatus();
    }
    else if (exceptionType == 3)
    {
        g_console_logger->debug("Point B alarm ");
        g_file_logger->debug("Point B alarm ");
        point_b.ResetStatus();
    }
}

int WashReport::GetScore(float p)
{
    int pp = p * 100;
    if (pp >= 0 && pp <= 25)
    {
        return 1;
    }
    else if (pp > 25 && pp <= 50)
    {
        return 2;
    }
    else if (pp > 50 && pp <= 75)
    {
        return 3;
    }
    else
    {
        return 4;
    }
    return 1;
}

void WashReport::StartHeartBeat()
{

    mHeartBearTimer.setInterval([&]()
                                {
                                    json res = GetDeviceStatusJson();
                                    res["status"] = 1;
                                    res["updateTime"] = getTime(time_format);

                                    PostJsonToServer(res); },
                                120 * 1000);

#if (DIRECTOR_LINK_ENABLE == 1)
    mDlReportStatusTimer.setInterval([&]()
                                     { dl_report_status(deviceNo, 0); },
                                     120 * 1000); // 5 minutes
#endif
}

void WashReport::SetDLWashFunc(dl_report_wash_func_t func)
{
    if (func != NULL)
    {
        dl_report_wash = func;
    }
    else
    {
        throw invalid_argument("func is null");
    }
}
void WashReport::SetDLCarPassFunc(dl_report_car_pass_func_t func)
{
    if (func != NULL)
    {
        dl_report_car_pass = func;
    }
    else
    {
        // TODO: throw exception
    }
}

void WashReport::SetDLStatusFunc(dl_report_status_func_t func)
{
    if (func)
        dl_report_status = func;
}