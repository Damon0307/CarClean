#include <iostream>
#include <sstream> // 添加sstream头文件
#include <thread>
#include <chrono>
#include <string>
#include <locale>
#include <codecvt>
#include <memory>

#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Text.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/XML/XMLWriter.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Document.h"
#include "DirectorLinkClient.h"
#include "spdlog/spdlog.h"

using Poco::Net::SocketAddress;

using namespace Poco::XML;
using namespace std;
using Poco::XML::AutoPtr;
using Poco::XML::Document;
using Poco::XML::DOMWriter;
using std::cout;
using std::stringstream;

using json = nlohmann::json;

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

void DirectorLinkClient::ConnectToserver()
{
    while (true)
    {
        try
        {
            Poco::Net::SocketAddress sa(_ip, _port);
            _socket->connect(sa);
            std::cout << "Connected to server" << std::endl;
            g_console_logger->info("dl clinet Connected to server");
            g_file_logger->info("dl clinet Connected to server");
            break;
        }
        catch (Poco::Net::ConnectionRefusedException &e)
        {
            std::cout << "Connection refused, retrying in 5 seconds..." << std::endl;
            g_console_logger->warn("dl clinet Connection refused, retrying in 5 seconds...");
            g_file_logger->warn("dl clinet Connection refused, retrying in 5 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

// 由于我们没有离开图片，所以目前只能上传冲洗时候的过车视频
void DirectorLinkClient::ReportCarPass(const json &data)
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode(xmbh_str));
    pData->appendChild(pXmbh);

    AutoPtr<Element> pCaptureTime = pDoc->createElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    pCaptureTime->appendChild(pDoc->createTextNode(p_capture_time));
    pData->appendChild(pCaptureTime);

    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);
    AutoPtr<Element> pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode(ztcCph_str));
    pData->appendChild(pZtcCph);

    AutoPtr<Element> pZtcColor = pDoc->createElement("ZtcColor");
    int ztcColor = convertCarColor(data);
    pZtcColor->appendChild(pDoc->createTextNode(std::to_string(ztcColor)));
    pData->appendChild(pZtcColor);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    pDeviceNo->appendChild(pDoc->createTextNode(pDeviceNo_str));
    pData->appendChild(pDeviceNo);

    AutoPtr<Element> pVehicleType = pDoc->createElement("VehicleType");
    int vehicleType_int = convertCarType(data);
    pVehicleType->appendChild(pDoc->createTextNode(std::to_string(vehicleType_int)));
    pData->appendChild(pVehicleType);

    // <PhotoUrl>https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg</PhotoUrl>
    AutoPtr<Element> pPhotoUrl = pDoc->createElement("PhotoUrl");
    //! 实时过车时候利用的是进入时间？
    std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";
    //?std::string enter_time_str = data["captureTime"];
    std::string enter_time_str = data["enterTime"];

    std::string dateOnly = enter_time_str.substr(0, 4) + enter_time_str.substr(5, 2) + enter_time_str.substr(8, 2);
    std::string dateTime = formatDateTime(enter_time_str);

    std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
    pPhotoUrl->appendChild(pDoc->createTextNode(photo_url_res));
    pData->appendChild(pPhotoUrl);

    // 创建一个DOMWriter实例。
    DOMWriter writer;
    stringstream ss;
    // 将XML文档写入stringstream。
    writer.writeNode(ss, pDoc);
    // 输出XML字符串。
    cout << ss.str() << std::endl;

    std::string xmlData = ss.str();
    // 手动添加XML声明 <?xml version="1.0" encoding="UTF-8" standalone="yes"?> 到stringstream的开头。
    xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;

    std::string header = "XZCC";
    std::string packetType = "GC";
    std::string dataToSend = header + packetType + xmlData;

    _socket->sendBytes(dataToSend.c_str(), dataToSend.length());
    g_console_logger->info("dl clinet ReportCarPass {}",dataToSend.c_str());
    g_file_logger->info("dl clinet ReportCarPass {}",dataToSend.c_str());   
}

void DirectorLinkClient::ReportCarWashInfo(const json &data, bool is_detour)
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode(xmbh_str));
    pData->appendChild(pXmbh);

    AutoPtr<Element> pCaptureTime = pDoc->createElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    pCaptureTime->appendChild(pDoc->createTextNode(p_capture_time));
    pData->appendChild(pCaptureTime);
    // ztcCph
    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);
    AutoPtr<Element> pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode(ztcCph_str));
    pData->appendChild(pZtcCph);

    AutoPtr<Element> pZtcColor = pDoc->createElement("ZtcColor");
    int ztcColor = convertCarColor(data);
    pZtcColor->appendChild(pDoc->createTextNode(std::to_string(ztcColor)));
    pData->appendChild(pZtcColor);

    AutoPtr<Element> pAlarmType = pDoc->createElement("AlarmType");
    int alarmType_int = convertToDLAlarmType(data);
    pAlarmType->appendChild(pDoc->createTextNode(std::to_string(alarmType_int)));
    pData->appendChild(pAlarmType);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    pDeviceNo->appendChild(pDoc->createTextNode(pDeviceNo_str));
    pData->appendChild(pDeviceNo);

    if (is_detour)
    {
        std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        std::string enter_time_str = data["captureTime"];
        std::string dateOnly = enter_time_str.substr(0, 4) + enter_time_str.substr(5, 2) + enter_time_str.substr(8, 2);
        std::string dateTime = formatDateTime(enter_time_str);
        AutoPtr<Element> pPhotoUrl = pDoc->createElement("PhotoUrl");
        pPhotoUrl->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pPhotoUrl);

        AutoPtr<Element> pPhotoUrlL = pDoc->createElement("LeftphotoUrl");
        pPhotoUrlL->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pPhotoUrlL);

        AutoPtr<Element> pPhotoUrlR = pDoc->createElement("RightphotoUrl");
        pPhotoUrlR->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pPhotoUrlR);

        AutoPtr<Element> VidioUrl = pDoc->createElement("VideoUrl");
        std::string video_url_res = video_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + "rx/" + dateTime + ".mp4";
        VidioUrl->appendChild(pDoc->createTextNode(video_url_res));
        pData->appendChild(VidioUrl);

        AutoPtr<Element> LeavededioUrl = pDoc->createElement("LeaveVideoUrl");
        LeavededioUrl->appendChild(pDoc->createTextNode(""));
        pData->appendChild(LeavededioUrl);
        // 绕行之填video_url字段其他都是空
    }
    else
    {
        AutoPtr<Element> pCleanRes = pDoc->createElement("CleanRes");
        int cleanres_int = data["cleanRes"];
        pCleanRes->appendChild(pDoc->createTextNode(std::to_string(cleanres_int)));
        pData->appendChild(pCleanRes);

        AutoPtr<Element> pVehicleType = pDoc->createElement("VehicleType");
        int vehicleType_int = convertCarType(data);
        pVehicleType->appendChild(pDoc->createTextNode(std::to_string(vehicleType_int)));
        pData->appendChild(pVehicleType);

        AutoPtr<Element> pLeftclean = pDoc->createElement("Leftclean");
        int leftclean_int = convertCarCleanResL(data);
        pLeftclean->appendChild(pDoc->createTextNode(std::to_string(leftclean_int)));
        pData->appendChild(pLeftclean);

        AutoPtr<Element> pRightclean = pDoc->createElement("Rightclean");
        int rightclean_int = convertCarCleanResR(data);
        pRightclean->appendChild(pDoc->createTextNode(std::to_string(rightclean_int)));
        pData->appendChild(pRightclean);

        std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";
        // 2.照片规则
        // http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong/20240302/MSCCM003/20240302131217.jpg
        // 红色部分为变量：
        // 20240302为上传数据json中的enterTime（进入时间）转为YYYYMMDD格式
        // MSCCM003为上传数据json中的deviceNo（设备号）
        // 20240302131217为上传数据json中的enterTime（进入时间）转为YYYYMMDDHHmmsS格式
        // ，其中如果是抓拍照片，后面什么也不加，如果是左轮照片，后面加L（20240302131217L），右轮照片，后面加R（20240302131217R）
        std::string enter_time_str = data["enterTime"];
        std::string dateOnly = enter_time_str.substr(0, 4) + enter_time_str.substr(5, 2) + enter_time_str.substr(8, 2);
        std::string dateTime = formatDateTime(enter_time_str);

        // std::string leave_time_str = data["leaveTime"];
        // std::string dateOnlyLeave = leave_time_str.substr(0, 4) + leave_time_str.substr(5, 2) + leave_time_str.substr(8, 2);
        // std::string dateTimeLeave = formatDateTime(leave_time_str);

        AutoPtr<Element> pPhotoUrl = pDoc->createElement("PhotoUrl");
        std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
        pPhotoUrl->appendChild(pDoc->createTextNode(photo_url_res));
        pData->appendChild(pPhotoUrl);

        AutoPtr<Element> pPhotoUrlL = pDoc->createElement("LeftphotoUrl");
        std::string leftphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "L.jpg";
        pPhotoUrlL->appendChild(pDoc->createTextNode(leftphoto_url_res));
        pData->appendChild(pPhotoUrlL);

        AutoPtr<Element> pPhotoUrlR = pDoc->createElement("RightphotoUrl");
        std::string rightphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "R.jpg";
        pPhotoUrlR->appendChild(pDoc->createTextNode(rightphoto_url_res));
        pData->appendChild(pPhotoUrlR);

        AutoPtr<Element> VidioUrl = pDoc->createElement("VideoUrl");
        std::string video_url_res = video_url_prefix + dateOnly + "/" + pDeviceNo_str + "/" + "zp" + "/" + dateTime + ".mp4";
        VidioUrl->appendChild(pDoc->createTextNode(video_url_res));
        pData->appendChild(VidioUrl);

        AutoPtr<Element> LeavededioUrl = pDoc->createElement("LeaveVideoUrl");
        std::string leave_video_url_res = video_url_prefix + dateOnly + "/" + pDeviceNo_str + "/" + "mk" + "/" + dateTime + ".mp4";
        LeavededioUrl->appendChild(pDoc->createTextNode(leave_video_url_res));
        pData->appendChild(LeavededioUrl);
    }

    // std::cout << "苏AXY377----->" << convertLicensePlate("川AXY377") << std::endl;
    // 创建一个DOMWriter实例。
    DOMWriter writer;
    stringstream ss;
    // 将XML文档写入stringstream。
    writer.writeNode(ss, pDoc);
    // 输出XML字符串。
    cout << ss.str() << std::endl;

    std::string xmlData = ss.str();
    // 手动添加XML声明 <?xml version="1.0" encoding="UTF-8" standalone="yes"?> 到stringstream的开头。
    xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;

    std::string header = "XZCC";
    std::string packetType = "GJ";
    std::string dataToSend = header + packetType + xmlData;

    _socket->sendBytes(dataToSend.c_str(), dataToSend.length());
    g_console_logger->info("dl clinet ReportCarWashInfo {}",dataToSend.c_str());
    g_file_logger->info("dl clinet ReportCarWashInfo {}",dataToSend.c_str());   
}

void DirectorLinkClient::ReportStatus(const std::string &device_no, int status)
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");

    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    pDeviceNo->appendChild(pDoc->createTextNode(device_no));
    pData->appendChild(pDeviceNo);

    //<Devstatus>0</Devstatus>//状态(0：正常 1：异常)
    AutoPtr<Element> pDevstatus = pDoc->createElement("Devstatus");
    pDevstatus->appendChild(pDoc->createTextNode(to_string(status)));
    pData->appendChild(pDevstatus);

    // 创建一个DOMWriter实例。
    DOMWriter writer;
    stringstream ss;
    // 将XML文档写入stringstream。
    writer.writeNode(ss, pDoc);
    // 输出XML字符串。
    cout << ss.str() << std::endl;
    std::string xmlData = ss.str();
    xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;

    std::string header = "XZCC";
    std::string packetType = "ZT";
    std::string dataToSend = header + packetType + xmlData;

    _socket->sendBytes(dataToSend.c_str(), dataToSend.length());
}

void DirectorLinkClient::RecvServerMessage()
{
    // 在一个线程中循环接收并处理消息
    while (true)
    {
        bool recv_res = receiveAndParseMessage(*_socket, messageType, xmlData);
        if (recv_res == false)
        {
            std::cout << "recv error" << std::endl;
            break;
        }
        else
        {
            std::cout << "recv success" << std::endl;
        }
    }
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

bool DirectorLinkClient::receiveAndParseMessage(Poco::Net::StreamSocket &socket, std::string &messageType, std::string &xmlData)
{
    try
    {
        // 假设协议头部固定为12字节：4字节帧头 + 2字节或8字节类型 + 数据体
        const int HEADER_SIZE = 4;
        const int TYPE_SIZE_SMALL = 2;
        const int TYPE_SIZE_LARGE = 8;
        Poco::Buffer<char> buffer(HEADER_SIZE + TYPE_SIZE_LARGE + 4096); // 预分配足够大的缓冲区，4096是XML数据的大致预估大小
        // 读取固定长度的头部和数据类型
        int bytesRead = socket.receiveBytes(buffer.begin(), HEADER_SIZE + TYPE_SIZE_LARGE);
        if (bytesRead < HEADER_SIZE + TYPE_SIZE_SMALL)
        {
            // 读取到的数据长度不足以包含头部和数据类型，返回错误
            return false;
        }
        // 检查帧头
        std::string frameHeader(buffer.begin(), buffer.begin() + HEADER_SIZE);
        if (frameHeader != "XZCC")
        {
            // 帧头不匹配
            return false;
        }
        // 解析数据包类型
        std::string type(buffer.begin() + HEADER_SIZE, buffer.begin() + HEADER_SIZE + TYPE_SIZE_SMALL);
        if (type == "GJ")
        {
            messageType = type;
            // 跳过2字节类型，开始读取XML数据
            bytesRead = socket.receiveBytes(buffer.begin(), buffer.size());
            if (bytesRead <= 0)
            {
                // 没有更多数据可读
                return false;
            }
            xmlData.append(buffer.begin(), buffer.begin() + bytesRead);
        }
        else
        {
            // 尝试将类型作为整数解析
            try
            {
                int typeId = std::stoi(type);
                messageType = std::to_string(typeId);
                // 跳过8字节类型，开始读取XML数据
                bytesRead = socket.receiveBytes(buffer.begin(), buffer.size());
                if (bytesRead <= 0)
                {
                    // 没有更多数据可读
                    return false;
                }
                xmlData.append(buffer.begin(), buffer.begin() + bytesRead);
            }
            catch (const std::invalid_argument &)
            {
                // 解析整数失败
                return false;
            }
        }
        // 这里可以添加XML数据的验证逻辑，例如检查是否有完整的XML根元素等
        std::cout << "xmlData->" << xmlData << std::endl;
        // 清空xmlData
        xmlData.clear();

        return true;
    }
    catch (Poco::Exception &exc)
    {
        std::cerr << "DirectorLinkClient::receiveAndParseMessage exception: " << exc.displayText() << std::endl;
        return false;
    }
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