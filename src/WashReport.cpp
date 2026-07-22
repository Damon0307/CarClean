#include "WashReport.h"
#include <future>

#define DIRECTOR_LINK_ENABLE 0
#define NORMAL_REPLY_TO_IPC 1

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

const unsigned char event_normal[] = {0x55, 0x01, 0x00, 0x00, 0x00, 0x00, 0xde, 0x31, 0xaa};
const unsigned char event_dirty[] = {0x55, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe2, 0x0d, 0xaa};

const char *time_format = "%Y-%m-%d %H:%M:%S";

/*****************************/

WashReport::WashReport(/* args */)
{
    has_barrier_gate = false;
    point_b.ResetStatus();
    point_b.SetPonintExit(true);
    point_b.SetPointID("B");
    water_pump.ResetStatus();
    ipc.ResetStatus();
    serial_data_queue.clear();

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

    // 测试时可通过环境变量指定串口设备（如 pty），默认使用 /dev/ttyS3
    const char *env_port = getenv("WASH_SERIAL_PORT");
    port_name = (env_port != nullptr) ? env_port : "/dev/ttyS3";

    serial_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (serial_fd == -1)
    {
        perror("Failed to open serial port");
    }
    struct termios options;

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

    if (data.contains("BarrierGate") && data["BarrierGate"])
    {
        mBarrierGate = new BarrierGate();
        has_barrier_gate = true;

        int p_delay_time = data["delay_time"];
        int p_keep_time = data["keep_time"];

        this->mDelayTimeMs = p_delay_time * 1000;
        this->mKeepTimeMs = p_keep_time * 1000;
    }
    else
    {
        mBarrierGate = NULL;
    }

    if (data.contains("time_after_b"))
    {
        ai_deal_delay_time = data["time_after_b"];
    }

    if (data.contains("power_type_report_interval"))
    {
        power_type_report_interval = data["power_type_report_interval"];
    }

    g_console_logger->debug("wash alarm time set to {}", wash_alarm_time);

    f.close();

    ReportPowerType();
    power_type_report_timer.setInterval([&]() {
        ReportPowerType();
    },
                                      power_type_report_interval * 60 * 1000);
}

// 接收到摄像头推送的抓拍数据
void WashReport::DealWashIPCData(const json &p_json, Response &res)
{
    json response = ResponseToIPC(NORMAL_REPLY_TO_IPC);
    res.set_content(response.dump(), "application/json");

    bool has_license = p_json.contains("AlarmInfoPlate") &&
                       p_json["AlarmInfoPlate"].contains("result") &&
                       p_json["AlarmInfoPlate"]["result"].contains("PlateResult") &&
                       p_json["AlarmInfoPlate"]["result"]["PlateResult"].contains("license");

    if (!has_license)
    {
        g_file_logger->debug("Got Wash IPC Data  NO  Licenses!!! ");
        g_console_logger->debug("Got Wash IPC Data NO  Licenses!!!  ");
        return;
    }

    g_file_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());
    g_console_logger->debug("Got Wash IPC Data {} ", p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump().c_str());

    {
        std::lock_guard<std::mutex> lk(sensor_data_mutex);

      // 有效车牌标记一辆新车周期的开始，清除上一周期状态
point_b.ResetStatus();
water_pump.ResetStatus();
ai_ipc_mgr.ResetAll();

ipc.json_data = p_json;
ipc.has_trigger = true;
time(&car_active_time);
    }

    std::cout << "Got Car license : " << p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"].dump() << std::endl;
}

// 接收到绕道摄像头推送的数据
void WashReport::DealDetourIPCData(const json &p_json, Response &res)
{
    json capture_res = GetCaptureJson();
    capture_res["captureTime"] = utc_to_string(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
    capture_res["ztcCph"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
    capture_res["ztcColor"] = CarColorConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
    capture_res["vehicleType"] = CarTypeConvert(p_json["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
    capture_res["picture"] = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];
    capture_res["enterTime"] = "";
    capture_res["leaveTime"] = "";

    capture_res["alarmType"] = 1;
    capture_res["frontWheelWashTime"] = 0;
    capture_res["hindWheelWashTime"] = 0;

    capture_res.erase("rightclean");
    capture_res.erase("leftclean");

    int ipc_dir = p_json["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];

    if (ipc_dir == 4)
    {
        capture_res["direction"] = 1;
        PostJsonToServer(capture_res);
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
    }

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

// === AI IPC 处理方法（当前启用: 左轮/右轮/车尾） ===

void WashReport::Deal_L_AIIPCData(const json &p_json, Response &res)
{
    std::lock_guard<std::mutex> lk(sensor_data_mutex);
    ai_ipc_mgr.DealAIIPCData(AIIPCManager::Position::LEFT_WHEEL, p_json,
                              point_b.is_working, point_b.leave_time,
                              ipc.has_trigger, ai_deal_delay_time);
    res.set_content("OK", "text/plain");
}

void WashReport::Deal_R_AIIPCData(const json &p_json, Response &res)
{
    std::lock_guard<std::mutex> lk(sensor_data_mutex);
    ai_ipc_mgr.DealAIIPCData(AIIPCManager::Position::RIGHT_WHEEL, p_json,
                              point_b.is_working, point_b.leave_time,
                              ipc.has_trigger, ai_deal_delay_time);
    res.set_content("OK", "text/plain");
}

void WashReport::Deal_Tail_AIIPCData(const json &p_json, Response &res)
{
    std::lock_guard<std::mutex> lk(sensor_data_mutex);
    ai_ipc_mgr.DealAIIPCData(AIIPCManager::Position::TAIL, p_json,
                              point_b.is_working, point_b.leave_time,
                              ipc.has_trigger, ai_deal_delay_time);
    res.set_content("OK", "text/plain");
}

/*
// === 暂时屏蔽: 车身两侧 ===
void WashReport::Deal_Side_L_AIIPCData(const json &p_json, Response &res)
{
    ai_ipc_mgr.DealAIIPCData(AIIPCManager::Position::SIDE_LEFT, p_json,
                              point_b.is_working, point_b.leave_time,
                              ipc.has_trigger, ai_deal_delay_time);
    res.set_content("OK", "text/plain");
}

void WashReport::Deal_Side_R_AIIPCData(const json &p_json, Response &res)
{
    ai_ipc_mgr.DealAIIPCData(AIIPCManager::Position::SIDE_RIGHT, p_json,
                              point_b.is_working, point_b.leave_time,
                              ipc.has_trigger, ai_deal_delay_time);
    res.set_content("OK", "text/plain");
}
*/

// 处理串口数据
// 帧头	开关量状态	CRC16	帧尾
// 0x55	5位二进制数	2字节CRC16校验值	0xAA
void WashReport::DealSerialData()
{
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
        // 超时
    }
    else
    {
        if (FD_ISSET(serial_fd, &readfds))
        {
            uint8_t buf[128] = {0};
            int buf_len = read(serial_fd, buf, sizeof(buf));

            if (buf_len > 0)
            {
                for (int i = 0; i < buf_len; i++)
                {
                    serial_data_queue.push_back(buf[i]);
                }

                if (serial_data_queue.size() >= 9)
                {
                    for (int i = 0; i < serial_data_queue.size(); i++)
                    {
                        if (serial_data_queue[i] == 0x55 && (i + 8) <= (serial_data_queue.size() - 1))
                        {
                            point_b.DealStatus(serial_data_queue[i + 2]);
                            water_pump.DealStatus(serial_data_queue[i + 5]);

                            // 电源类型
                            int power_type = serial_data_queue[i + 1];
                            if (power_type != cur_power_type)
                            {
                                cur_power_type = power_type;
                                g_console_logger->info("Power type changed to {} ", cur_power_type);
                                g_file_logger->info("Power type changed to {} ", cur_power_type);
                                ReportPowerType();
                            }

                            break;
                        }
                    }
                    std::deque<char> zero;
                    serial_data_queue.swap(zero);
                }
            }
        }
    }
}

void WashReport::SetPassJsonFunc(std::function<bool(json)> func)
{
    PostJsonToServer = func;
}

#ifdef WASH_TEST_MODE
void WashReport::InjectSerialFrame(const std::deque<char> &frame)
{
    std::lock_guard<std::mutex> lk(sensor_data_mutex);

    serial_data_queue = frame;
    if (serial_data_queue.size() >= 9)
    {
        for (size_t i = 0; i < serial_data_queue.size(); i++)
        {
            if (serial_data_queue[i] == 0x55 && (i + 8) <= (serial_data_queue.size() - 1))
            {
                point_b.DealStatus(serial_data_queue[i + 2]);
                water_pump.DealStatus(serial_data_queue[i + 5]);

                int power_type = serial_data_queue[i + 1];
                if (power_type != cur_power_type)
                {
                    cur_power_type = power_type;
                    g_console_logger->info("Power type changed to {} ", cur_power_type);
                    g_file_logger->info("Power type changed to {} ", cur_power_type);
                    ReportPowerType();
                }
                break;
            }
        }
        std::deque<char> zero;
        serial_data_queue.swap(zero);
    }
}

json WashReport::GetSensorStatusJson()
{
    std::lock_guard<std::mutex> lk(sensor_data_mutex);

    json res;
    res["point_b"]["is_working"] = point_b.is_working;
    res["point_b"]["cur_status"] = point_b.cur_status;
    res["point_b"]["exit_car_leaving"] = point_b.exit_car_leaving;
    res["point_b"]["trigger_time"] = point_b.trigger_time;
    res["point_b"]["leave_time"] = point_b.leave_time;

    res["water_pump"]["is_working"] = water_pump.is_working;
    res["water_pump"]["begin_time"] = water_pump.begin_time;
    res["water_pump"]["finish_time"] = water_pump.finish_time;

    res["ipc"]["has_trigger"] = ipc.has_trigger;

    res["ai_ipc"]["left_ready"] = ai_ipc_mgr.IsLeftWheelReady();
    res["ai_ipc"]["right_ready"] = ai_ipc_mgr.IsRightWheelReady();
    res["ai_ipc"]["tail_ready"] = ai_ipc_mgr.IsTailReady();

    return res;
}
#endif

json WashReport::GetCaptureJson()
{
    json res;
    res["xmbh"] = "XMBH00000003";
    res["deviceNo"] = deviceNo;
    res["captureTime"] = "";
    res["ztcCph"] = "";
    res["ztcColor"] = "";
    res["vehicleType"];
    res["enterTime"] = "";
    res["leaveTime"] = "";
    res["alarmType"] = 5;

    res["frontWheelWashTime"] = 0;
    res["hindWheelWashTime"] = 0;
    res["deviceSerial"] = nvr_serial_num;
    res["localIndex"] = nvr_channel;
    res["picture"] = " ";
    res["dataType"] = 1;
    res["direction"];
    res["cleanRes"] = 0;
    res["leftphotoUrl"] = "";
    res["rightphotoUrl"] = "";
    res["rightclean"] = 0;
    res["leftclean"] = 0;
    res["gate_status"] = 0;
    res["open_time"] = "";

    // 车尾图片/洁净度
    res["tailPic"] = "";
    res["tailCleanLevel"] = 0;

    /*
    // 暂时屏蔽: 顶棚+车身两侧
    res["isCoverd"] = false;
    res["roofPic"] = "";
    res["leftPic"] = "";
    res["rightPic"] = "";
    res["leftCleanLevel"] = 0;
    res["rightCleanLevel"] = 0;
    */

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

bool WashReport::GetAIIPCDetectResult()
{
    return ai_ipc_mgr.BothWheelsReady();
}

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
        res["Response_AlarmInfoPlate"]["is_pay"] = true;
    }
    break;
    default:
        break;
    }
    return res;
}

// 主处理循环
void WashReport::StartReportingProcess()
{
    printf("WashReporter StartReportingProcess...\n");
    StartHeartBeat();

    bool last_point_b_status = true;
    bool last_point_b_working = true;
    bool exit_car_leaving = true;

    while (1)
    {
        DealSerialData();

        bool should_report = false;
        json ipc_json_copy;
        time_t leave_time_copy = 0;

        {
            std::lock_guard<std::mutex> lk(sensor_data_mutex);

            if (ipc.has_trigger == true)
            {
                if (point_b.is_working != last_point_b_working || point_b.cur_status != last_point_b_status || point_b.exit_car_leaving != exit_car_leaving)
                {
                    g_console_logger->debug("B Working Status Leaving {} {} {}", static_cast<int>(point_b.is_working), static_cast<int>(point_b.cur_status), static_cast<int>(point_b.exit_car_leaving));
                    last_point_b_working = point_b.is_working;
                    last_point_b_status = point_b.cur_status;
                    exit_car_leaving = point_b.exit_car_leaving;
                }

                if ((point_b.is_working) && (point_b.IsLeaving() == true))
                {
                    should_report = true;
                    ipc_json_copy = ipc.json_data;
                    leave_time_copy = point_b.leave_time;
                }
            }
        }

        if (should_report)
        {
            json capture_res = GetCaptureJson();
            capture_res["captureTime"] = utc_to_string(ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);
            capture_res["ztcCph"] = ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["license"];
            capture_res["ztcColor"] = CarColorConvert(ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["colorType"]);
            capture_res["vehicleType"] = CarTypeConvert(ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["type"]);
            capture_res["enterTime"] = utc_to_string(ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"]);

            double diff_seconds = difftime(leave_time_copy, car_active_time);
            long long time_interval = static_cast<long long>(diff_seconds);

            long long base_time = ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["timeStamp"]["Timeval"]["sec"].get<long long>();
            long long final_leave_time = base_time + time_interval;

            capture_res["leaveTime"] = utc_to_string(final_leave_time);

            std::cout << "enter time: " << capture_res["enterTime"] << std::endl;
            std::cout << "leave time: " << capture_res["leaveTime"] << std::endl;

            capture_res["alarmType"] = GetAlarmByWaterPump();

            g_console_logger->debug("leave time: {}", capture_res["leaveTime"].dump().c_str());
            g_file_logger->debug("leave time: {}", capture_res["leaveTime"].dump().c_str());

            capture_res["frontWheelWashTime"] = 0;
            capture_res["hindWheelWashTime"] = 0;

            capture_res["picture"] = ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["imageFile"];

            int ipc_dir = ipc_json_copy["AlarmInfoPlate"]["result"]["PlateResult"]["direction"];
            capture_res["direction"] = GetDirByIPC(ipc_dir);

            // AI数据收集窗口：完整等待 ai_deal_delay_time 秒
            // 窗口内可能收到多帧 clean/dirty，全部累积到AIIPC状态中
            // 窗口结束后再做最终判断（任出现过一帧dirty即为脏车）
            static constexpr int AI_POLL_INTERVAL_MS = 200;

            bool ai_all_res = false;
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(ai_deal_delay_time);

            g_console_logger->debug("collecting ai ipc data, full {}s window, poll {}ms",
                                    ai_deal_delay_time, AI_POLL_INTERVAL_MS);
            g_file_logger->debug("collecting ai ipc data, full {}s window, poll {}ms",
                                 ai_deal_delay_time, AI_POLL_INTERVAL_MS);

            while (std::chrono::steady_clock::now() < deadline)
            {
                auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now()).count();
                // 最后一次睡眠不超过剩余时间，避免溢出窗口
                auto sleep_ms = std::min<long long>(AI_POLL_INTERVAL_MS, remaining);
                if (sleep_ms <= 0) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            }

            ai_all_res = GetAIIPCDetectResult();

            g_console_logger->debug("AI IPC data collect finished with result {} ", ai_all_res);
            g_file_logger->debug("AI IPC data collect finished with result {} ", ai_all_res);

            // === 处理左右车轮AI结果（核心判断） ===
            if (ai_all_res)
            {
                json l_detect_json_data = ai_ipc_mgr.GetLeftWheelDetectRes();
                json r_detect_json_data = ai_ipc_mgr.GetRightWheelDetectRes();

                std::string l_label = l_detect_json_data.contains("label") ? l_detect_json_data["label"] : "unknown";
                std::string r_label = r_detect_json_data.contains("label") ? r_detect_json_data["label"] : "unknown";

                if (r_label == "clean" && l_label == "clean")
                {
                    capture_res["cleanRes"] = 2;

                    g_console_logger->debug("All clean {}", capture_res["ztcCph"].dump().c_str());
                    g_file_logger->debug("All clean {}", capture_res["ztcCph"].dump().c_str());

                    if (NULL != mBarrierGate)
                    {
                        g_console_logger->debug("Gate Will be open for {} in {} ms ", capture_res["ztcCph"].dump().c_str(), mDelayTimeMs);
                        g_file_logger->debug("Gate Will be open for {} in {} ms ", capture_res["ztcCph"].dump().c_str(), mDelayTimeMs);

                        mBarrierGate->BarrierGateCtrl(false);
                        mDelayTimer.setTimeout([this]()
                                               { mBarrierGate->BarrierGateCtrl(true); }, mDelayTimeMs);
                        mKeepTimer.setTimeout([this]()
                                              { mBarrierGate->BarrierGateCtrl(false); }, mDelayTimeMs + mKeepTimeMs);
                    }
                    if (has_barrier_gate)
                    {
                        capture_res["gate_status"] = 1;
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
                        capture_res["gate_status"] = 2;
                    else
                        capture_res["gate_status"] = 0;
                }

                std::string l_photo_url = l_detect_json_data.contains("img_base64") ? l_detect_json_data["img_base64"] : "";
                std::string r_photo_url = r_detect_json_data.contains("img_base64") ? r_detect_json_data["img_base64"] : "";
                capture_res["leftphotoUrl"] = l_photo_url;
                capture_res["rightphotoUrl"] = r_photo_url;

                float l_score = l_detect_json_data.contains("score") ? l_detect_json_data["score"].get<float>() : 0.0;
                float r_score = r_detect_json_data.contains("score") ? r_detect_json_data["score"].get<float>() : 0.0;
                capture_res["leftclean"] = l_score;
                capture_res["rightclean"] = r_score;
            }
            else
            {
                capture_res["cleanRes"] = 1;
                capture_res["leftclean"] = 0;
                capture_res["rightclean"] = 0;
                g_console_logger->debug("Timeout occurred while waiting for ai ipc data");
                g_file_logger->debug("Timeout occurred while waiting for ai ipc data");
            }

            /*
            // === 暂时屏蔽: 左右两侧车身 ===
            if (ai_ipc_mgr.IsSideLeftReady() && ai_ipc_mgr.IsSideRightReady())
            {
                auto l_result = ai_ipc_mgr.GetSideLeftDetectRes();
                auto r_result = ai_ipc_mgr.GetSideRightDetectRes();

                capture_res["leftPic"] = l_result.contains("img_base64") ? l_result["img_base64"] : "";
                capture_res["rightPic"] = r_result.contains("img_base64") ? r_result["img_base64"] : "";
                capture_res["leftCleanLevel"] = l_result.contains("score") ? l_result["score"].get<float>() : 0.0;
                capture_res["rightCleanLevel"] = r_result.contains("score") ? r_result["score"].get<float>() : 0.0;
            }
            */

            // === 车尾数据 ===
            if (ai_ipc_mgr.IsTailReady())
            {
                auto tail_result = ai_ipc_mgr.GetTailDetectRes();
                capture_res["tailPic"] = tail_result.contains("img_base64") ? tail_result["img_base64"] : "";
                capture_res["tailCleanLevel"] = tail_result.contains("score") ? tail_result["score"].get<float>() : 0.0;
            }
            else
            {
                g_console_logger->debug("Tail AI IPC data not ready, skipping tail clean level and picture");
                g_file_logger->debug("Tail AI IPC data not ready, skipping tail clean level and picture");
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

            {
                std::lock_guard<std::mutex> lk(sensor_data_mutex);
                ResetAllSensor();
            }

            std::cout << "===================Pass and reset===================" << std::endl;
            g_file_logger->debug("===================Pass and reset===================");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void WashReport::NotificationsToUart(int event_num)
{
    if (event_num == 1)
    {
        UART_Send(serial_fd, (char *)event_normal, sizeof(event_normal));
    }
    else if (event_num == 2)
    {
        UART_Send(serial_fd, (char *)event_dirty, sizeof(event_dirty));
    }
}

void WashReport::ResetAllSensor()
{
    point_b.ResetStatus();
    water_pump.ResetStatus();
    ipc.ResetStatus();
    ai_ipc_mgr.ResetAll();
}

int WashReport::GetAlarmByWaterPump()
{
    const time_t def_time = 0;

    if (water_pump.begin_time == 0)
    {
        return 3;
    }
    if (water_pump.begin_time != 0 && water_pump.finish_time == 0)
    {
        if (difftime(point_b.leave_time, water_pump.begin_time) > wash_alarm_time)
            return 5;
        else
            return 2;
    }

    if (water_pump.begin_time != def_time && water_pump.finish_time != def_time)
    {
        if (difftime(water_pump.finish_time, water_pump.begin_time) > wash_alarm_time)
            return 5;
        else
            return 2;
    }
    return 4;
}

int WashReport::GetDirByIPC(int ipc_dir)
{
    if (ipc_dir == 4)
        return 1;
    else if (ipc_dir == 3)
        return 0;
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
    res["alarmTime"] = oss.str();
    res["exceptionType"] = exceptionType;
    res["dataType"] = 3;
    PostJsonToServer(res);

    if (exceptionType == 1)
    {
        g_console_logger->debug("Point A alarm ");
        g_file_logger->debug("Point A alarm ");
    }
    else if (exceptionType == 2)
    {
        g_console_logger->debug("Water_pump alarm ");
        g_file_logger->debug("Water_pump alarm ");
        water_pump.ResetStatus();
    }
    else if (exceptionType == 3)
    {
        g_console_logger->debug("Point B alarm ");
        g_file_logger->debug("Point B alarm ");
        point_b.ResetStatus();
    }
}

void WashReport::ReportPowerType()
{
    json res;
    res["deviceNo"] = deviceNo;
    res["powerType"] = cur_power_type;
    res["dataType"] = 5;
    res["updateTime"] = getTime(time_format);

    if (PostJsonToServer)
    {
        PostJsonToServer(res);
    }
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
                                     120 * 1000);
#endif
}

void WashReport::SetDLWashFunc(dl_report_wash_func_t func)
{
    if (func != NULL)
        dl_report_wash = func;
    else
        throw invalid_argument("func is null");
}

void WashReport::SetDLCarPassFunc(dl_report_car_pass_func_t func)
{
    if (func != NULL)
        dl_report_car_pass = func;
}

void WashReport::SetDLStatusFunc(dl_report_status_func_t func)
{
    if (func)
        dl_report_status = func;
}
