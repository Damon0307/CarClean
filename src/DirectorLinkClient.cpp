#include <iostream>
#include <sstream> // 添加sstream头文件
#include <thread>
#include <chrono>
#include <string>
#include <locale>
#include <codecvt>
#include <memory>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "DirectorLinkClient.h"
#include "spdlog/spdlog.h"

#define BUFFER_SIZE  1024

using namespace std;

using std::cout;
using std::stringstream;
using namespace tinyxml2;
using json = nlohmann::json;

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

void DirectorLinkClient::ConnectToserver()
{
 

    std::string ip = "36.156.64.198";
    int port = 11011;

    // std::string ip = "192.168.169.1";
    // int port = 9090;

    int max_attempts = 5;
    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0)
        {
            std::cerr << "创建套接字失败\n";
            exit(-1);
        }
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr));
        if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "连接失败，尝试 #" << attempt << "\n";
            close(socket_fd);
            // 如果是ECONNREFUSED错误，可能需要稍等一下
            if (attempt < max_attempts)
            {
                sleep(1);
            }
            continue;
        }
        else
        {
            std::cout << "成功连接到 " << ip << ":" << port << "\n";
            break;
        }
    }
}
int DirectorLinkClient::SendData(const std::string &data)
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if (socket_fd < 0)
    {
        std::cerr << "无效的套接字描述符 尝试重连\n";
       ConnectToserver();   
    }
    g_console_logger->info("dl client SendData in...{}",data);
    g_file_logger->info("dl client SendData in...{}",data);

    int bytes_sent = send(socket_fd, data.c_str(), data.length(), 0);
    if (bytes_sent < 0)
    {
        std::cerr << "发送数据失败\n";
        socket_fd =-1;
        return -1;
    }
    return bytes_sent;
}

void DirectorLinkClient::ReportStatus(const std::string &device_no, int status)
{
    g_console_logger->info("dl clinet ReportStatus in...");
    XMLDocument doc;
    // 创建XML声明（可选，因为XMLDocument会自动添加默认声明）
    XMLDeclaration* declaration = doc.NewDeclaration();
   // declaration->SetValue("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    declaration->SetValue("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");

    doc.InsertFirstChild(declaration);
  
    XMLElement *root = doc.NewElement("Data");
    doc.InsertAfterChild(declaration, root);
    // 确保token_str是类的成员变量并且已经被正确初始化
    XMLElement *child = doc.NewElement("Token");
    child->SetText(token_str.c_str());
    root->InsertEndChild(child);
    child = doc.NewElement("DeviceNo");
    child->SetText(device_no.c_str());
    root->InsertEndChild(child);
    child = doc.NewElement("Devstatus");
    // 直接使用std::string，避免不必要的转换
    child->SetText(std::to_string(status).c_str());
    root->InsertEndChild(child);
    // 序列化XML文档
    XMLPrinter printer;
    doc.Accept(&printer);
    std::string xmlStr = printer.CStr();
    std::string header = "XZCC";
    std::string packetType = "ZT";
    std::string dataToSend = header + packetType + xmlStr;
    // 发送数据，确保有错误处理
    if (!SendData(dataToSend))
    {
        g_console_logger->error("Failed to send data");
        // 处理错误...
    }
    g_console_logger->info("dl client ReportStatus out...{}",dataToSend);
}

void DirectorLinkClient::ReportCarPass(const json &data, bool is_in)
{
    g_console_logger->info("dl clinet ReportCarPass  in...");
    // 创建XML文档并设置内容
      XMLDocument doc;
    // 创建XML声明（可选，因为XMLDocument会自动添加默认声明）
    XMLDeclaration* declaration = doc.NewDeclaration();
    //declaration->SetValue("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
   // declaration->SetValue("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    declaration->SetValue("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    doc.InsertFirstChild(declaration);
  
    XMLElement *root = doc.NewElement("Data");
    doc.InsertAfterChild(declaration, root);
    // 确保token_str是类的成员变量并且已经被正确初始化
    XMLElement *child = doc.NewElement("Token");
    child->SetText(token_str.c_str());
    root->InsertEndChild(child);
    // 创建子元素并设置文本内容

    child = doc.NewElement("Xmbh");
    child->SetText(xmbh_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    child->SetText(p_capture_time.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("ztcCph");
    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);
    child->SetText(ztcCph_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("ztcColor");
    int ztcColor = convertCarColor(data);
    child->SetText(std::to_string(ztcColor).c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    child->SetText(pDeviceNo_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("VehicleType");
    int vehicleType_int = convertCarType(data);
    child->SetText(std::to_string(vehicleType_int).c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("PhotoUrl");
    std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";
    std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
    std::string dateTime = formatDateTime(p_capture_time);
    std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
    child->SetText(photo_url_res.c_str());
    root->InsertEndChild(child);

    // 序列化XML文档
    XMLPrinter printer;
    doc.Accept(&printer);
    std::string xmlStr = printer.CStr();
    std::string header = "XZCC";
    std::string packetType = "GC";

    std::string dataToSend = header + packetType + xmlStr;
    // 发送数据，确保有错误处理
    if (!SendData(dataToSend))
    {
        g_console_logger->error("Failed to send data");
        // 处理错误...
    }
    g_console_logger->info("dl client ReportCarPass out...");
}

void DirectorLinkClient::ReportCarWashInfo(const json &data, bool is_detour)
{
    g_file_logger->info("dl clinet ReportCarWashInfo in...");

      XMLDocument doc;
    // 创建XML声明（可选，因为XMLDocument会自动添加默认声明）
    XMLDeclaration* declaration = doc.NewDeclaration();
    //declaration->SetValue("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    declaration->SetValue("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    doc.InsertFirstChild(declaration);
  
    XMLElement *root = doc.NewElement("Data");
    doc.InsertAfterChild(declaration, root);
    // 确保token_str是类的成员变量并且已经被正确初始化
    XMLElement *child = doc.NewElement("Token");
    child->SetText(token_str.c_str());
    root->InsertEndChild(child);
    // 创建子元素并设置文本内容

    child = doc.NewElement("Xmbh");
    child->SetText(xmbh_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    child->SetText(p_capture_time.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("ZtcCph");
    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);
    child->SetText(ztcCph_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("ZtcColor");
    int ztcColor = convertCarColor(data);
    child->SetText(std::to_string(ztcColor).c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    child->SetText(pDeviceNo_str.c_str());
    root->InsertEndChild(child);

    child = doc.NewElement("AlarmType");
    int alarmType_int = convertToDLAlarmType(data);
    child->SetText(std::to_string(alarmType_int).c_str());
    root->InsertEndChild(child);

    if (is_detour)
    {
        static std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        static std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";

        std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
        std::string dateTime = formatDateTime(p_capture_time);

        child = doc.NewElement("PhotoUrl");
        std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
        child->SetText(photo_url_res.c_str());
        root->InsertEndChild(child);
        child = doc.NewElement("LeftphotoUrl");
        child->SetText("");
        root->InsertEndChild(child);
        child = doc.NewElement("RightphotoUrl");
        child->SetText("");
        root->InsertEndChild(child);

        child = doc.NewElement("VideoUrl");
        std::string video_url_res = video_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + "rx/" + dateTime + ".mp4";
        child->SetText(video_url_res.c_str());
        root->InsertEndChild(child);
        child = doc.NewElement("LeaveVideoUrl");
        child->SetText("");
        root->InsertEndChild(child);
    }
    else
    {
        child = doc.NewElement("CleanRes");
        int cleanres_int = data["cleanRes"];
        child->SetText(std::to_string(cleanres_int).c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("VehicleType");
        int vehicleType_int = convertCarType(data);
        child->SetText(std::to_string(vehicleType_int).c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("Leftclean");
        int leftclean_int = convertCarCleanResL(data);
        child->SetText(std::to_string(leftclean_int).c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("Rightclean");
        int rightclean_int = convertCarCleanResR(data);
        child->SetText(std::to_string(rightclean_int).c_str());
        root->InsertEndChild(child);

        static std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        static std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";

        std::string video_time = data["enterTime"];

        std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
        std::string dateTime = formatDateTime(p_capture_time);

        child = doc.NewElement("PhotoUrl");
        std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
        child->SetText(photo_url_res.c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("LeftphotoUrl");
        std::string leftphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "L.jpg";
        child->SetText(leftphoto_url_res.c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("RightphotoUrl");
        std::string rightphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "R.jpg";
        child->SetText(rightphoto_url_res.c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("VideoUrl");
        std::string video_url_res = video_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + "zp" + "/" + dateTime + ".mp4";
        child->SetText(video_url_res.c_str());
        root->InsertEndChild(child);

        child = doc.NewElement("LeaveVideoUrl");
        std::string leave_video_url_res = video_url_prefix + dateOnly + "/" + pDeviceNo_str + "/" + "mk" + "/" + dateTime + ".mp4";
        child->SetText(leave_video_url_res.c_str());
        root->InsertEndChild(child);
    }

    // 序列化XML文档
    XMLPrinter printer;
    doc.Accept(&printer);
    std::string xmlStr = printer.CStr();

    std::string header = "XZCC";
    std::string packetType = "GJ";
    std::string dataToSend = header + packetType + xmlStr;

    if (!SendData(dataToSend))
    {
        g_console_logger->error("Failed to send data");
        // 处理错误...
    }
    g_file_logger->info("dl client ReportCarWashInfo out...");
}

std::string DirectorLinkClient::formatDateTime(const std::string &dateTime)
{
    std::string formattedDateTime;
    for (char c : dateTime)
    {
        if (c != '-' && c != ' ' && c != ':')
        {
            formattedDateTime += c;
        }
    }
    return formattedDateTime;
}
std::string DirectorLinkClient::addSeconds(const std::string &dateTime, int secondsToAdd)
{
    int hours = std::stoi(dateTime.substr(11, 2));
    int minutes = std::stoi(dateTime.substr(14, 2));
    int seconds = std::stoi(dateTime.substr(17, 2));

    seconds += secondsToAdd;

    // 处理秒数溢出
    minutes += seconds / 60;
    seconds %= 60;
    hours += minutes / 60;
    minutes %= 60;
    hours %= 24;

    // 构造新的日期时间字符串
    std::string newDateTime = dateTime.substr(0, 11) +
                              (hours < 10 ? "0" : "") + std::to_string(hours) + ":" +
                              (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                              (seconds < 10 ? "0" : "") + std::to_string(seconds);

    return newDateTime;
}

int DirectorLinkClient::convertToDLAlarmType(const json &data)
{
    // 判断data中有没有alarmType字段，然后根据其值进行转化
    if (data.contains("alarmType"))
    {
        int alarm_type = data["alarmType"];
        switch (alarm_type)
        {
        case 1:
            return 1;
        case 2:
            return 2;
        case 3:
            return 3;
        case 4:
            return 4;
        case 5:
            return 5;
        default:
            return 0;
        }
    }
    return 0;
}
// 车牌号转换
std::string DirectorLinkClient::convertLicensePlate(const std::string &licensePlate)
{
    static const std::string provinces[] = {"京", "津", "冀", "晋", "蒙", "辽", "吉", "黑", "沪", "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "渝", "川", "贵", "云", "藏", "陕", "甘", "青", "宁", "新", "台", "港", "澳"};
    static const int provincesLength = sizeof(provinces) / sizeof(provinces[0]);
    // Convert the input string to wstring for proper Unicode handling
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideLicensePlate = converter.from_bytes(licensePlate);
    if (wideLicensePlate.length() >= 1)
    {
        for (int i = 0; i < provincesLength; ++i)
        {
            std::wstring province = converter.from_bytes(provinces[i]);
            if (wideLicensePlate.substr(0, 1) == province)
            {
                // Convert the index to a string and ensure it is two digits
                std::wstring index = (i < 10) ? L"0" + std::to_wstring(i) : std::to_wstring(i);
                // Concatenate the index with the rest of the license plate
                wideLicensePlate = index + wideLicensePlate.substr(1);
                break;
            }
        }
    }
    // Convert the resulting wstring back to string
    return converter.to_bytes(wideLicensePlate);
}

// 转换车牌颜色
int DirectorLinkClient::convertCarColor(const json &data)
{
    // 判断data中有没有ztcColor字段，然后根据其值进行转化
    if (data.contains("ztcColor"))
    {
        int color_type = data["ztcColor"];
        switch (color_type)
        {
        case 1:
            return 3;
        case 2:
            return 2;
        case 3:
            return 1;
        case 5:
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}
// 转换车辆类型
int DirectorLinkClient::convertCarType(const json &data)
{
    // 判断data 中有没有 vehicleType 字段进行车辆类型转换
    if (data.contains("vehicleType"))
    {
        int vehicle_type = data["vehicleType"];
        switch (vehicle_type)
        {
        case 1:
            return 2;
        case 2:
            return 1;
        default:
            return 99;
        }
    }
    return -1;
}

int DirectorLinkClient::convertCarCleanResL(const json &data)
{
    // 判断data 中有没有 leftclean 字段进行车辆类型转换 如果有进行乘以100，以25一档来转换
    if (data.contains("leftclean"))
    {
        int leftclean = (int)(data["leftclean"]);
        leftclean *= 100;
        if (leftclean > 0 && leftclean <= 25)
        {
            return 4;
        }
        else if (leftclean > 25 && leftclean <= 50)
        {
            return 3;
        }
        else if (leftclean > 50 && leftclean <= 75)
        {
            return 2;
        }
        else if (leftclean > 75 && leftclean <= 100)
        {
            return 1;
        }
    }
    return -1;
}

int DirectorLinkClient::convertCarCleanResR(const json &data)
{
    // 判断data 中有没有 rightclean 字段进行车辆类型转换 如果有进行乘以100，以25一档来转换
    if (data.contains("rightclean"))
    {
        int rightclean = data["rightclean"];
        rightclean *= 100;
        if (rightclean > 0 && rightclean <= 25)
        {
            return 4;
        }
        else if (rightclean > 25 && rightclean <= 50)
        {
            return 3;
        }
        else if (rightclean > 50 && rightclean <= 75)
        {
            return 2;
        }
        else if (rightclean > 75 && rightclean <= 100)
        {
            return 1;
        }
    }
    return -1;
}

 void DirectorLinkClient::receiveAndParseMessage()
{
    g_console_logger->info("dl client receiveAndParseMessage in...");

    while (true) { // 这里仍然使用无限循环，你需要根据实际情况添加退出条件
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        // 使用阻塞式recv等待接收数据
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0)
        {
            std::string str(buffer);
            g_console_logger->info("Received message: {}", str);
            // 在这里解析和处理接收到的消息
        }
        else if (bytes_received == 0)
        {
            // 连接已关闭
            g_console_logger->info("Connection closed by peer");
            break; // 或者继续，取决于你的连接关闭策略
        }
        else
        {
            // recv错误
            g_console_logger->error("recv failed: {}", strerror(errno));
            break; // 或者继续，取决于你的错误处理策略
        }
        // 使用sleep模拟处理消息的过程
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 根据需要添加退出循环的条件
        // 例如，检查某个标志位或者处理特定消息后退出
    }
}