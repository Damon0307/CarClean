#include "WashReport.h"
#include <future>
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
    point_a.ResetStatus();
    point_b.ResetStatus();
    point_b.SetPonintExit(true); // 设置B点为出口点
    point_a.SetPointID("A");
    point_b.SetPointID("B");
    water_pump.ResetStatus();
    ipc.ResetStatus();
    serial_data_queue.clear();
    // need lock?
    point_a.alarm_func = std::bind(&WashReport::AlarmReport, this, std::placeholders::_1);
    point_b.alarm_func = std::bind(&WashReport::AlarmReport, this, std::placeholders::_1);
    water_pump.alarm_func = std::bind(&WashReport::AlarmReport, this, std::placeholders::_1);
}

WashReport::~WashReport()
{
}

void WashReport::InitSerialComm(const char *file_path)
{
    std::ifstream f(file_path);
    json data = json::parse(f);

    serial_fd = -1;
    port_name = data["port"];

    std::cout << "Going to open port " << port_name << std::endl;
    serial_fd = UART_Open(serial_fd, (char *)port_name.c_str()); // 打开串口，返回文件描述符

    // 设置串口数据帧格式
    if (UART_Set(serial_fd, 115200, 0, 8, 1, 'N') == FALSE)
    {
        printf("serial set err\n");
        exit(-1);
    }
    printf("Set Port Exactly!\n");
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

    g_console_logger->debug("wash alarm time set to {}", wash_alarm_time);

    f.close();
}

// 接收到摄像头推送的抓拍数据
void WashReport::DealWashIPCData(const json &p_json, Response &res)
{

    // std::cout << p_json.dump() << std::endl;
    ipc.json_data = p_json;
    ipc.has_trigger = true;
    json response = ResponseToIPC(NORMAL_REPLY_TO_IPC);
    res.set_content(response.dump(), "application/json");

    if (p_json.contains("AlarmInfoPlate") && p_json["AlarmInfoPlate"].contains("result") && p_json["AlarmInfoPlate"]["result"].contains("PlateResult") && p_json["AlarmInfoPlate"]["result"]["PlateResult"].contains("license"))
    {
        g_file_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());
        g_console_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());
    }
    else
    {
        g_file_logger->debug("Got Wash IPC Data  NO  Licenses!!! ");
        g_console_logger->debug("Got Wash IPC Data NO  Licenses!!!  ");
    }
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

// 处理两侧车轮冲洗干净程度的数据
void WashReport::Deal_L_AIIPCData(const json &p_json, Response &res)
{
    if (p_json.contains("label"))
    {
        if (point_a.is_working)
        {
            l_ai_ipc.DealAIIPCData(p_json);
            g_console_logger->debug("Deal_L_AIIPCData {} ", p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_L_AIIPCData {}", p_json["label"].dump().c_str());
        }
        else
        {
            g_console_logger->debug("Rejected handle cause no point a working");
            g_file_logger->debug("Rejected handle cause no point a working");
        }
    }

    res.set_content("OK", "text/plain");
}
void WashReport::Deal_R_AIIPCData(const json &p_json, Response &res)
{

    if (p_json.contains("label"))
    {
        if (point_a.is_working)
        {
            r_ai_ipc.DealAIIPCData(p_json);
            g_console_logger->debug("Deal_R_AIIPCData {} ", p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_R_AIIPCData {}", p_json["label"].dump().c_str());
        }
        else
        {
            g_console_logger->debug("Rejected handle cause no point a working");
            g_file_logger->debug("Rejected handle cause no point a working");
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
                        if (serial_data_queue[i] = 0x55 && (i + 8) <= (serial_data_queue.size() - 1))
                        { // 寻找到帧头，且后续长度足够解
                            have_decode = true;
                            // 校验CRC16
                            point_a.DealStatus(serial_data_queue[i + 1]);
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

void WashReport::SetPassJsonFunc(std::function<void(json)> func)
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
    res["ztcColor"];
    res["vehicleType"];
    res["enterTime"] = "";
    res["leaveTime"] = "";
    res["alarmType"];
    res["frontWheelWashTime"] = 0;
    res["hindWheelWashTime"] = 0;
    res["deviceSerial"] = nvr_serial_num;
    res["localIndex"] = nvr_channel;
    res["picture"] = "";
    res["dataType"] = 1;
    res["direction"];
    res["cleanRes"];          // 车辆车轮清洗结果 1：未知  2：冲洗干净  3：未冲洗干净
    res["leftphotoUrl"] = ""; // 车辆左侧抓拍图片
    res["rightphotoUrl"] = "";
    res["rightclean"] = 0;
    res["leftclean"] = 0; // 车辆左侧冲洗洁净 度数值

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

int WashReport::GetDirByCompareTime(const Point &a, const Point &b) // 通过比较两个点的先后时间得到方向
{
    int res = 0;
    if (a.trigger_time == 0 || b.trigger_time == 0) // 如果a,b 中存在一个点为0，说明该流程至少有一点没有触发，即流程未结束，或压根没开始
    {
        res = 0;
    }
    else if (a.trigger_time < b.trigger_time)
    {
        res = 1;
    }
    else if (a.trigger_time > b.trigger_time)
    {
        res = 2;
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
    printf("WashReporter StartReportingProcess...\n");
    StartHeartBeat();
    bool last_point_a_working = true;
    bool last_point_a_status = true;
    bool last_point_b_status = true;
    bool last_point_b_working = true;
    bool exit_car_leaving = true;

    while (1)
    {
        DealSerialData();

        if (point_a.is_working != last_point_a_working || point_a.cur_status != last_point_a_status)
        {
            // printf("A working  ,A status %d  %d \n", point_a.is_working, point_a.cur_status);
            g_console_logger->debug("A Working  Status {}   {}", static_cast<int>(point_a.is_working), static_cast<int>(point_a.cur_status));
            last_point_a_working = point_a.is_working;
            last_point_a_status = point_a.cur_status;
        }

        //   static_cast<int>

        if (point_b.is_working != last_point_b_working || point_b.cur_status != last_point_b_status || point_b.exit_car_leaving != exit_car_leaving)
        {
            // printf("A working  ,A status %d  %d \n", point_a.is_working, point_a.cur_status);
            g_console_logger->debug("B Working  Status  Leaving {}  {}  {} ", static_cast<int>(point_b.is_working), static_cast<int>(point_b.cur_status), static_cast<int>(point_b.exit_car_leaving));
            last_point_b_working = point_b.is_working;
            last_point_b_status = point_b.cur_status;
            exit_car_leaving = point_b.exit_car_leaving;
        }
        // printf("B working  ,B status  B leaving  %d  %d  %d \n", point_b.is_working, point_b.cur_status, point_b.exit_car_leaving);

        if ((point_b.is_working) && (point_b.IsLeaving() == true)) // B点触发，且下降沿
        {
            if (ipc.has_trigger == true) // IPC已经有推送结果则开始处理
            {

                // 组符合后端服务器的JSON
                json capture_res = GetCaptureJson(); // 已经包含默认信息
                capture_res["captureTime"] = utc_to_string(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
                capture_res["ztcCph"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
                capture_res["ztcColor"] = CarColorConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
                capture_res["vehicleType"] = CarTypeConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
                capture_res["enterTime"] = time_to_string(point_a.trigger_time);
                capture_res["leaveTime"] = time_to_string(point_b.leave_time);
                capture_res["alarmType"] = GetAlarmTypeByPoint();
                // 前后轮冲洗时间改为 0
                capture_res["frontWheelWashTime"] = water_pump.finish_time - water_pump.begin_time;
                capture_res["hindWheelWashTime"] = water_pump.finish_time - water_pump.begin_time;
                capture_res["picture"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];
                int ipc_dir = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];
                capture_res["direction"] = GetDirByIPC(ipc_dir); // 通过IPC

                g_console_logger->debug("Leaving with Report A Working {} {}", static_cast<int>(point_a.is_working), capture_res["ztcCph"].dump().c_str());
                g_file_logger->debug("Leaving with Report A Working {} {}", static_cast<int>(point_a.is_working), capture_res["ztcCph"].dump().c_str());

                bool ai_all_res = false;
                for (int i = 0; i < 10; i++)
                {
                    printf("waiting for  ai ipc data... \n");
                    ai_all_res = GetAIIPCDetectResult();
                    if (ai_all_res == true)
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
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
                    }
                    else
                    {
                        capture_res["cleanRes"] = 3;
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
                PostJsonToServer(capture_res);
                // NotificationsToUart
                if (capture_res["cleanRes"] == 2)
                {
                    NotificationsToUart(2);
                }
                else
                {
                    NotificationsToUart(1);
                }

                // todo 检查推送结果以后再决定要不要重传？
                ResetAllSensor();
                g_console_logger->debug("===================Pass and reset===================");
                g_file_logger->debug("===================Pass and reset===================");
            }
            else
            {
                g_console_logger->debug("Leaving without report no wash ipc data");
                g_file_logger->debug("Leaving without report no wash ipc data");

                // TODO b点出发了，但是没有捕捉到车牌需要重置传感器
                ResetAllSensor();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
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

std::string WashReport::time_to_string(time_t t)
{
    std::string result(20, '\0'); // 分配足够的空间来存储时间字符串
    std::strftime(&result[0], result.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    // std::cout << " time_to_string " << result << std::endl;
    result.resize(std::strlen(result.c_str())); // 调整字符串的长度以去除多余的空字符
    return result;
}

std::string WashReport::utc_to_string(long long utcSeconds)
{
    // 获取UTC时间
    std::time_t utcTime(utcSeconds);
    // 转换为本地时间
    std::tm *localTime = std::localtime(&utcTime);
    // 设置时区为北京时间 (+8小时)
    // localTime->tm_hour += 8;
    // 转换为字符串
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void WashReport::ResetAllSensor()
{
    point_a.ResetStatus();
    point_b.ResetStatus();
    water_pump.ResetStatus();
    ipc.ResetStatus();
    l_ai_ipc.ResetStatus();
    r_ai_ipc.ResetStatus();
}

// todo  其他未定义，绕道未处理
int WashReport::GetAlarmTypeByPoint()
{
    // 水泵未工作
    if (water_pump.is_working == false)
    {
        return 3;
    }
    // 冲洗时间不足  wash_alarm_time S
    if (point_b.leave_time - point_a.trigger_time < wash_alarm_time)
    {
        return 2;
    }
    return 5;
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
        point_a.ResetStatus();
    }
    else if (exceptionType == 2)
    {
        water_pump.ResetStatus();
    }
    else if (exceptionType == 3)
    {
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
}