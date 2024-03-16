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
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/Node.h"
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

void DirectorLinkClient::ReportCarPass(const json &data, bool is_in)
{
   static std::unique_ptr<DOMParser> parser;
   static AutoPtr<Document> pDoc;

   static AutoPtr<Element> pData;
    static AutoPtr<Element> pToken;
    static AutoPtr<Element> pXmbh;
    static AutoPtr<Element> pCaptureTime;
    static AutoPtr<Element> pZtcCph;
    static AutoPtr<Element> pZtcColor;
    static AutoPtr<Element> pDeviceNo;
    static AutoPtr<Element> pVehicleType;
    static AutoPtr<Element> pPhotoUrl;
    static  DOMWriter writer;
    std::stringstream ss;
    std::string xmlData;
    std::lock_guard<std::mutex> lock(mtx);
    if (!parser)
    {
        parser = std::make_unique<DOMParser>();
    }
    if (!pDoc)
    {
        pDoc = parser->parseString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>");
    }
 
    if (!pData)
    {
        pData = pDoc->documentElement();
    }
    if (!pToken)
    {
        pToken = pDoc->createElement("Token");
    }
 
    // 获取根元素
    pData = pDoc->documentElement();

      // 清除 pData 下的所有子元素
    AutoPtr<NodeList> childNodes = pData->childNodes();
    while (childNodes->length() > 0) {
        pData->removeChild(childNodes->item(0));
    }

    // 创建子元素并设置文本内容
    pToken = pDoc->createElement("Token");
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode(xmbh_str));
    pData->appendChild(pXmbh);

    pCaptureTime = pDoc->createElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    pCaptureTime->appendChild(pDoc->createTextNode(p_capture_time));
    pData->appendChild(pCaptureTime);

    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);

    pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode(ztcCph_str));
    pData->appendChild(pZtcCph);

    pZtcColor = pDoc->createElement("ZtcColor");
    int ztcColor = convertCarColor(data);
    pZtcColor->appendChild(pDoc->createTextNode(std::to_string(ztcColor)));
    pData->appendChild(pZtcColor);

    pDeviceNo = pDoc->createElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    pDeviceNo->appendChild(pDoc->createTextNode(pDeviceNo_str));
    pData->appendChild(pDeviceNo);

    pVehicleType = pDoc->createElement("VehicleType");
    int vehicleType_int = convertCarType(data);
    pVehicleType->appendChild(pDoc->createTextNode(std::to_string(vehicleType_int)));
    pData->appendChild(pVehicleType);

    // <PhotoUrl>https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg</PhotoUrl>
    pPhotoUrl = pDoc->createElement("PhotoUrl");
    //! 现在都改成用captureTime
    std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";

    std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
    std::string dateTime = formatDateTime(p_capture_time);

    std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
    pPhotoUrl->appendChild(pDoc->createTextNode(photo_url_res));
    pData->appendChild(pPhotoUrl);
 
    // 将XML文档写入stringstream。
    writer.writeNode(ss, pDoc);
  
    xmlData = ss.str();
    // 手动添加XML声明 <?xml version="1.0" encoding="UTF-8" standalone="yes"?> 到stringstream的开头。
    xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;

    std::string header = "XZCC";
    std::string packetType = "GC";
    std::string dataToSend = header + packetType + xmlData;

    _socket->sendBytes(dataToSend.c_str(), dataToSend.length());
    g_console_logger->info("dl clinet ReportCarPass {}", dataToSend.c_str());
    g_file_logger->info("dl clinet ReportCarPass {}", dataToSend.c_str());
    
    // 清除 pToken 的所有子节点
    AutoPtr<NodeList> tokenChildNodes = pToken->childNodes();
    while (tokenChildNodes->length() > 0)
    {
        pToken->removeChild(tokenChildNodes->item(0));
         // 手动释放节点内存
    }
      
    // 清除 pDeviceNo 的所有子节点
    AutoPtr<NodeList> deviceNoChildNodes = pDeviceNo->childNodes();
    while (deviceNoChildNodes->length() > 0)
    {
        pDeviceNo->removeChild(deviceNoChildNodes->item(0));
    }
  

    AutoPtr<NodeList> xmbhChildNodes = pXmbh->childNodes();
    while (xmbhChildNodes->length() > 0)
    {
        pXmbh->removeChild(xmbhChildNodes->item(0));
    }
   


    AutoPtr<NodeList> captureTimeChildNodes = pCaptureTime->childNodes();
    while (captureTimeChildNodes->length() > 0)
    {
        pCaptureTime->removeChild(captureTimeChildNodes->item(0));
    }
  


    AutoPtr<NodeList> ztcCphChildNodes = pZtcCph->childNodes();
    while (ztcCphChildNodes->length() > 0)
    {
        pZtcCph->removeChild(ztcCphChildNodes->item(0));
    }
 

    AutoPtr<NodeList> ztcColorChildNodes = pZtcColor->childNodes();
    while (ztcColorChildNodes->length() > 0)
    {
        pZtcColor->removeChild(ztcColorChildNodes->item(0));
    }
   


    AutoPtr<NodeList> vehicleTypeChildNodes = pVehicleType->childNodes();
    while (vehicleTypeChildNodes->length() > 0)
    {
        pVehicleType->removeChild(vehicleTypeChildNodes->item(0));
    }
 
    AutoPtr<NodeList> photoUrlChildNodes = pPhotoUrl->childNodes();
    while (photoUrlChildNodes->length() > 0)
    {
        pPhotoUrl->removeChild(photoUrlChildNodes->item(0));
    }
  

}

void DirectorLinkClient::ReportCarWashInfo(const json &data, bool is_detour)
{
     
    static std::unique_ptr<DOMParser> parser;
    static AutoPtr<Document> pDoc;
    static AutoPtr<Element> pData;
    static AutoPtr<Element> pToken;
    static AutoPtr<Element> pDeviceNo;
    static AutoPtr<Element> pDevstatus;
    static AutoPtr<Element> pXmbh;
    static AutoPtr<Element> pCaptureTime;
    static AutoPtr<Element> pZtcCph;
    static AutoPtr<Element> pZtcColor;
    static AutoPtr<Element> pAlarmType;
    static AutoPtr<Element> pLeftclean;
    static AutoPtr<Element> pRightclean;
    static AutoPtr<Element> pVehicleType;
    static AutoPtr<Element> pPhotoUrl;
    static AutoPtr<Element> pPhotoUrlL;
    static AutoPtr<Element> pPhotoUrlR;
    static AutoPtr<Element> pVideoUrl;
    static AutoPtr<Element> pLeavededioUrl;
    static AutoPtr<Element> pCleanRes;
    static DOMWriter writer;

    std::lock_guard<std::mutex> lock(mtx);

    if (!parser)
    {
        parser = std::make_unique<DOMParser>();
    }
    if (!pDoc)
    {
        pDoc = parser->parseString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>");
    }
    if (!pData)
    {
        pData = pDoc->documentElement();
    }
    if (!pToken)
    {
        pToken = pDoc->createElement("Token");
    }

    g_file_logger->info("dl clinet ReportCarWashInfo in...");
 

    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    pDoc = parser.get()->parseString(xmlString);

  
    // 获取根元素
    pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    pToken = pDoc->createElement("Token");
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode(xmbh_str));
    pData->appendChild(pXmbh);

    

    pCaptureTime = pDoc->createElement("CaptureTime");
    std::string p_capture_time = data["captureTime"];
    pCaptureTime->appendChild(pDoc->createTextNode(p_capture_time));
    pData->appendChild(pCaptureTime);


    // ztcCph
    std::string ztcCph_str = data["ztcCph"];
    ztcCph_str = convertLicensePlate(ztcCph_str);
    pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode(ztcCph_str));
    pData->appendChild(pZtcCph);

    pZtcColor = pDoc->createElement("ZtcColor");
    int ztcColor = convertCarColor(data);
    pZtcColor->appendChild(pDoc->createTextNode(std::to_string(ztcColor)));
    pData->appendChild(pZtcColor);

    pAlarmType = pDoc->createElement("AlarmType");
    int alarmType_int = convertToDLAlarmType(data);
    pAlarmType->appendChild(pDoc->createTextNode(std::to_string(alarmType_int)));
    pData->appendChild(pAlarmType);

    pDeviceNo = pDoc->createElement("DeviceNo");
    std::string pDeviceNo_str = data["deviceNo"];
    pDeviceNo->appendChild(pDoc->createTextNode(pDeviceNo_str));
    pData->appendChild(pDeviceNo);



    if (is_detour)
    {
        static std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        static std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";

        std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
        std::string dateTime = formatDateTime(p_capture_time);

        pPhotoUrl = pDoc->createElement("PhotoUrl");
        std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
        pPhotoUrl->appendChild(pDoc->createTextNode(photo_url_res));
        pData->appendChild(pPhotoUrl);

        pPhotoUrlL = pDoc->createElement("LeftphotoUrl");
        pPhotoUrlL->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pPhotoUrlL);

        pPhotoUrlR = pDoc->createElement("RightphotoUrl");
        pPhotoUrlR->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pPhotoUrlR);

        pVideoUrl = pDoc->createElement("VideoUrl");
        std::string video_url_res = video_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + "rx/" + dateTime + ".mp4";
        pVideoUrl->appendChild(pDoc->createTextNode(video_url_res));
        pData->appendChild(pVideoUrl);

        pLeavededioUrl = pDoc->createElement("LeaveVideoUrl");
        pLeavededioUrl->appendChild(pDoc->createTextNode(""));
        pData->appendChild(pLeavededioUrl);
    }
    else
    {
        pCleanRes = pDoc->createElement("CleanRes");
        int cleanres_int = data["cleanRes"];
        pCleanRes->appendChild(pDoc->createTextNode(std::to_string(cleanres_int)));
        pData->appendChild(pCleanRes);

        pVehicleType = pDoc->createElement("VehicleType");
        int vehicleType_int = convertCarType(data);
        pVehicleType->appendChild(pDoc->createTextNode(std::to_string(vehicleType_int)));
        pData->appendChild(pVehicleType);

        pLeftclean = pDoc->createElement("Leftclean");
        int leftclean_int = convertCarCleanResL(data);
        pLeftclean->appendChild(pDoc->createTextNode(std::to_string(leftclean_int)));
        pData->appendChild(pLeftclean);

        pRightclean = pDoc->createElement("Rightclean");
        int rightclean_int = convertCarCleanResR(data);
        pRightclean->appendChild(pDoc->createTextNode(std::to_string(rightclean_int)));
        pData->appendChild(pRightclean);

        static std::string video_url_prefix = "http://36.156.67.46:9001/updateApi/FileDownload/downloadFile?fileName=";
        static std::string photo_url_prefix = "http://36.156.67.46:8237/prod-api/thirdPlatFace/getCheChongImg?fileName=/profile/chechong";

        std::string video_time = data["enterTime"];

        std::string dateOnly = p_capture_time.substr(0, 4) + p_capture_time.substr(5, 2) + p_capture_time.substr(8, 2);
        std::string dateTime = formatDateTime(p_capture_time);

        pPhotoUrl = pDoc->createElement("PhotoUrl");
        std::string photo_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + ".jpg";
        pPhotoUrl->appendChild(pDoc->createTextNode(photo_url_res));
        pData->appendChild(pPhotoUrl);

        pPhotoUrlL = pDoc->createElement("LeftphotoUrl");
        std::string leftphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "L.jpg";
        pPhotoUrlL->appendChild(pDoc->createTextNode(leftphoto_url_res));
        pData->appendChild(pPhotoUrlL);

        pPhotoUrlR = pDoc->createElement("RightphotoUrl");
        std::string rightphoto_url_res = photo_url_prefix + "/" + dateOnly + "/" + pDeviceNo_str + "/" + dateTime + "R.jpg";
        pPhotoUrlR->appendChild(pDoc->createTextNode(rightphoto_url_res));
        pData->appendChild(pPhotoUrlR);

        pVideoUrl = pDoc->createElement("VideoUrl");
        std::string video_url_res = video_url_prefix + dateOnly + "/" + pDeviceNo_str + "/" + "zp" + "/" + dateTime + ".mp4";
        pVideoUrl->appendChild(pDoc->createTextNode(video_url_res));
        pData->appendChild(pVideoUrl);

        pLeavededioUrl = pDoc->createElement("LeaveVideoUrl");
        std::string leave_video_url_res = video_url_prefix + dateOnly + "/" + pDeviceNo_str + "/" + "mk" + "/" + dateTime + ".mp4";
        pLeavededioUrl->appendChild(pDoc->createTextNode(leave_video_url_res));
        pData->appendChild(pLeavededioUrl);
    }

    // std::cout << "苏AXY377----->" << convertLicensePlate("川AXY377") << std::endl;
    // 创建一个DOMWriter实例。

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
    // g_console_logger->info("dl clinet ReportCarWashInfo {}",dataToSend.c_str());
    g_file_logger->info("dl clinet ReportCarWashInfo {}", dataToSend.c_str());
    g_file_logger->info("dl clinet ReportCarWashInfo out...");

#if 0
    AutoPtr<NodeList> tokenChildNodes = pToken->childNodes();
    while (tokenChildNodes->length() > 0)
    {
        pToken->removeChild(tokenChildNodes->item(0));
    }
    AutoPtr<NodeList> deviceNoChildNodes = pDeviceNo->childNodes();
    while (deviceNoChildNodes->length() > 0)
    {
        pDeviceNo->removeChild(deviceNoChildNodes->item(0));
    }
    AutoPtr<NodeList> devstatusChildNodes = pDevstatus->childNodes();
    while (devstatusChildNodes->length() > 0)
    {
        pDevstatus->removeChild(devstatusChildNodes->item(0));
    }
    AutoPtr<NodeList> xmbhChildNodes = pXmbh->childNodes();
    while (xmbhChildNodes->length() > 0)
    {
        pXmbh->removeChild(xmbhChildNodes->item(0));
    }
    #endif

}

void DirectorLinkClient::ReportStatus(const std::string &device_no, int status)
{
    static std::unique_ptr<DOMParser> parser;
    static AutoPtr<Document> pDoc;
    static AutoPtr<Element> pData;
    static AutoPtr<Element> pToken;
    static AutoPtr<Element> pDeviceNo;
    static AutoPtr<Element> pDevstatus;
    static DOMWriter writer;
    std::stringstream ss;
    std::string xmlData;
    std::lock_guard<std::mutex> lock(mtx);

    if (!parser)
    {
        parser = std::make_unique<DOMParser>();
    }
    if (!pDoc)
    {
        pDoc = parser->parseString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>");
    }
    if (!pData)
    {
        pData = pDoc->documentElement();
    }
    if (!pToken)
    {
        pToken = pDoc->createElement("Token");
    }
    if (!pDeviceNo)
    {
        pDeviceNo = pDoc->createElement("DeviceNo");
    }
    if (!pDevstatus)
    {
        pDevstatus = pDoc->createElement("Devstatus");
    }

    g_file_logger->info("dl clinet ReportStatus in...");

    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    pDeviceNo->appendChild(pDoc->createTextNode(device_no));
    pData->appendChild(pDeviceNo);

    //<Devstatus>0</Devstatus>//状态(0：正常 1：异常)

    pDevstatus->appendChild(pDoc->createTextNode(to_string(status)));
    pData->appendChild(pDevstatus);

    // 创建一个DOMWriter实例。

    // 将XML文档写入stringstream。
    writer.writeNode(ss, pDoc);

    // 输出XML字符串。
    cout << ss.str() << std::endl;
    xmlData = ss.str();
    xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;

    std::string header = "XZCC";
    std::string packetType = "ZT";
    std::string dataToSend = header + packetType + xmlData;

    _socket->sendBytes(dataToSend.c_str(), dataToSend.length());

    // const element = pDoc.get()->getElementById
    // element.innerHTML = "";

    // 清除 pToken 的所有子节点
    AutoPtr<NodeList> tokenChildNodes = pToken->childNodes();
    while (tokenChildNodes->length() > 0)
    {
        pToken->removeChild(tokenChildNodes->item(0));
    }
    // 清除 pDeviceNo 的所有子节点
    AutoPtr<NodeList> deviceNoChildNodes = pDeviceNo->childNodes();
    while (deviceNoChildNodes->length() > 0)
    {
        pDeviceNo->removeChild(deviceNoChildNodes->item(0));
    }
    // 清除 pDevstatus 的所有子节点
    AutoPtr<NodeList> devstatusChildNodes = pDevstatus->childNodes();
    while (devstatusChildNodes->length() > 0)
    {
        pDevstatus->removeChild(devstatusChildNodes->item(0));
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

void DirectorLinkClient::receiveAndParseMessage()
{
    char buffer[1024];
    std::string data;

    g_console_logger->info("dl client receiveAndParseMessage in...");

    // 设置 socket 为阻塞模式
    _socket->setBlocking(true); // Changed to blocking mode

    int totalBytesReceived = 0;
    int bytesReceived = 0;

    // 不断尝试读取数据，直到没有数据可读或达到退出条件
    while (true)
    {
        // 尝试读取数据
        bytesReceived = _socket->receiveBytes(buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            totalBytesReceived += bytesReceived;
            data.append(buffer, bytesReceived);
        }
        else
        {
            // 没有数据可读，退出循环
            break;
        }
    }

    // 如果有数据接收到
    if (totalBytesReceived > 0)
    {
        g_console_logger->info("dl client receiveAndParseMessage out...{}", data.c_str());
        g_file_logger->info("dl client receiveAndParseMessage out...{}", data.c_str());
    }
    else
    {
        // 没有数据接收到
        // g_console_logger->error("dl client receiveAndParseMessage failed...");
        // g_file_logger->error("dl client receiveAndParseMessage failed...");
    }

    data.clear();
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    g_console_logger->info("dl client receiveAndParseMessage out...");
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