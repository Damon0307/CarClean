#include "WashReport.h"
#include <future>
#define NORMAL_REPLY_TO_IPC 1
// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

const char *time_format = "%Y-%m-%d %H:%M:%S";

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

    ipc.ResetStatus();
}

WashReport::~WashReport()
{
 
}

void WashReport::InitSerialComm(const char *file_path)
{
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
        if (ipc.has_trigger)
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
        if (ipc.has_trigger)
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
#if 0
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
#endif
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
    // unsigned short crc = 0xFFFF;

    // while (len--)
    // {
    //     crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
    // }

    // return (crc);
}

// 处理数据
void WashReport::StartReportingProcess()
{
    printf("WashReporter StartReportingProcess...\n");
    StartHeartBeat();

    while (1)
    {
        if (ipc.has_trigger == true) // IPC已经有推送结果则开始处理
        {

            // 组符合后端服务器的JSON
            json capture_res = GetCaptureJson(); // 已经包含默认信息
            capture_res["captureTime"] = utc_to_string(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
            capture_res["ztcCph"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
            capture_res["ztcColor"] = CarColorConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
            capture_res["vehicleType"] = CarTypeConvert(ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
            // capture_res["enterTime"] = time_to_string(point_a.trigger_time);
            // capture_res["leaveTime"] = time_to_string(point_b.leave_time);
            //capture_res["alarmType"] = GetAlarmTypeByPoint();
            // 前后轮冲洗时间改为 0
            // capture_res["frontWheelWashTime"] = water_pump.finish_time - water_pump.begin_time;
            // capture_res["hindWheelWashTime"] = water_pump.finish_time - water_pump.begin_time;
            capture_res["picture"] = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];
            int ipc_dir = ipc.json_data["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];
            capture_res["direction"] = GetDirByIPC(ipc_dir); // 通过IPC

            bool ai_all_res = false;
            for (int i = 0; i < 10; i++)
            {

                g_console_logger->debug("waiting for  ai ipc data...");
                ai_all_res = GetAIIPCDetectResult();
                if (ai_all_res == true)
                {
                    g_console_logger->debug("Get AI ipc data");
                    g_file_logger->debug("Get AI ipc data");
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

            // todo 检查推送结果以后再决定要不要重传？
            ResetAllSensor();
            g_console_logger->debug("===================Pass and reset===================");
            g_file_logger->debug("===================Pass and reset===================");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

    ipc.ResetStatus();
    l_ai_ipc.ResetStatus();
    r_ai_ipc.ResetStatus();
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
