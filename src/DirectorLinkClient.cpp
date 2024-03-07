
#include <iostream>
#include <sstream> // 添加sstream头文件
#include <thread>
#include <chrono>
#include <string>
#include <locale>
#include <codecvt>

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

using Poco::Net::SocketAddress;

using namespace Poco::XML;
using namespace std;
using Poco::XML::AutoPtr;
using Poco::XML::Document;
using Poco::XML::DOMWriter;
using std::cout;
using std::stringstream;

using json = nlohmann::json;

void DirectorLinkClient::ConnectToserver()
{
    while (true)
    {
        try
        {
            Poco::Net::SocketAddress sa(_ip, _port);
            _socket->connect(sa);
            std::cout << "Connected to server" << std::endl;
            // std::string message = "hello_wjc";
            // _socket->sendBytes(message.data(), (int)message.size());
            break;
        }
        catch (Poco::Net::ConnectionRefusedException &e)
        {
            std::cout << "Connection refused, retrying in 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void DirectorLinkClient::ReportCarPass(const json &data)
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");
    // set pToken value to "1234"
    std::string token_str = "fuck it ";
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode("2019320105_3_1_1111"));
    pData->appendChild(pXmbh);

    AutoPtr<Element> pCaptureTime = pDoc->createElement("CaptureTime");
    pCaptureTime->appendChild(pDoc->createTextNode("2023-10-25 13:46:17"));
    pData->appendChild(pCaptureTime);

    AutoPtr<Element> pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode("09+ A123456"));
    pData->appendChild(pZtcCph);

    AutoPtr<Element> pZtcColor = pDoc->createElement("ZtcColor");
    pZtcColor->appendChild(pDoc->createTextNode("1"));
    pData->appendChild(pZtcColor);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    pDeviceNo->appendChild(pDoc->createTextNode("XXXX0001"));
    pData->appendChild(pDeviceNo);

    AutoPtr<Element> pVehicleType = pDoc->createElement("VehicleType");
    pVehicleType->appendChild(pDoc->createTextNode("01"));
    pData->appendChild(pVehicleType);

    // <PhotoUrl>https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg</PhotoUrl>
    AutoPtr<Element> pPhotoUrl = pDoc->createElement("PhotoUrl");
    pPhotoUrl->appendChild(pDoc->createTextNode("https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg"));
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
}

void DirectorLinkClient::ReportCarWashInfo(const json &data,bool is_detour)
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");
    // set pToken value to "1234"
    std::string token_str = "fuck it ";
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pXmbh = pDoc->createElement("Xmbh");
    pXmbh->appendChild(pDoc->createTextNode("2019320105_3_1_1111"));
    pData->appendChild(pXmbh);

    AutoPtr<Element> pCaptureTime = pDoc->createElement("CaptureTime");
    pCaptureTime->appendChild(pDoc->createTextNode("2023-10-25 13:46:17"));
    pData->appendChild(pCaptureTime);

    AutoPtr<Element> pZtcCph = pDoc->createElement("ZtcCph");
    pZtcCph->appendChild(pDoc->createTextNode("09+ A123456"));
    pData->appendChild(pZtcCph);

    AutoPtr<Element> pZtcColor = pDoc->createElement("ZtcColor");
    pZtcColor->appendChild(pDoc->createTextNode("1"));
    pData->appendChild(pZtcColor);

    AutoPtr<Element> pAlarmType = pDoc->createElement("AlarmType");
    pAlarmType->appendChild(pDoc->createTextNode("1"));
    pData->appendChild(pAlarmType);

    if(is_detour)
    {
       //绕行上报的比较少
    }else
    {

    }

    AutoPtr<Element> pCleanRes = pDoc->createElement("CleanRes");
    pCleanRes->appendChild(pDoc->createTextNode("1"));
    pData->appendChild(pCleanRes);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    pDeviceNo->appendChild(pDoc->createTextNode("XXXX0001"));
    pData->appendChild(pDeviceNo);

    AutoPtr<Element> pVehicleType = pDoc->createElement("VehicleType");
    pVehicleType->appendChild(pDoc->createTextNode("01"));
    pData->appendChild(pVehicleType);

    AutoPtr<Element> pLeftclean = pDoc->createElement("Leftclean");
    pLeftclean->appendChild(pDoc->createTextNode("1"));
    pData->appendChild(pLeftclean); // 车辆左侧冲洗洁净度数值(详见附录B)

    // 车辆右侧冲洗洁净度数值(详见附录B)
    AutoPtr<Element> pRightclean = pDoc->createElement("Rightclean");
    pRightclean->appendChild(pDoc->createTextNode("2"));
    pData->appendChild(pRightclean);


    // <PhotoUrl>https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg</PhotoUrl>
    AutoPtr<Element> pPhotoUrl = pDoc->createElement("PhotoUrl");
    pPhotoUrl->appendChild(pDoc->createTextNode("https://oss-xzzhgd.oss-cn-hangzhou.aliyuncs.com/11264930.jpg"));
    pData->appendChild(pPhotoUrl);

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
}
 

void DirectorLinkClient::ReportStatus()
{
    DOMParser parser;
    std::string xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Data></Data>";
    AutoPtr<Document> pDoc = parser.parseString(xmlString);
    // 获取根元素
    AutoPtr<Element> pData = pDoc->documentElement();
    // 创建子元素并设置文本内容
    AutoPtr<Element> pToken = pDoc->createElement("Token");
    // set pToken value to "1234"
    std::string token_str = "fuck it ";
    pToken->appendChild(pDoc->createTextNode(token_str)); // 将文本节点添加到元素节点中
    pData->appendChild(pToken);

    AutoPtr<Element> pDeviceNo = pDoc->createElement("DeviceNo");
    pDeviceNo->appendChild(pDoc->createTextNode("XXXX0001"));
    pData->appendChild(pDeviceNo);

    //<Devstatus>0</Devstatus>//状态(0：正常 1：异常)
    AutoPtr<Element> pDevstatus = pDoc->createElement("Devstatus");
    pDevstatus->appendChild(pDoc->createTextNode("0"));
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
        }else
        {
            std::cout << "recv success" << std::endl;
        }
    }
}

int DirectorLinkClient::convertToDLAlarmType(const json &data)
{
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
}
// 转换车辆类型
int DirectorLinkClient::convertCarType(const json &data)
{
}

bool DirectorLinkClient::receiveAndParseMessage(Poco::Net::StreamSocket &socket, std::string &messageType, std::string &xmlData)
{
  try {
        // 假设协议头部固定为12字节：4字节帧头 + 2字节或8字节类型 + 数据体
        const int HEADER_SIZE = 4;
        const int TYPE_SIZE_SMALL = 2;
        const int TYPE_SIZE_LARGE = 8;
        Poco::Buffer<char> buffer(HEADER_SIZE + TYPE_SIZE_LARGE + 4096); // 预分配足够大的缓冲区，4096是XML数据的大致预估大小
        // 读取固定长度的头部和数据类型
        int bytesRead = socket.receiveBytes(buffer.begin(), HEADER_SIZE + TYPE_SIZE_LARGE);
        if (bytesRead < HEADER_SIZE + TYPE_SIZE_SMALL) {
            // 读取到的数据长度不足以包含头部和数据类型，返回错误
            return false;
        }
        // 检查帧头
        std::string frameHeader(buffer.begin(), buffer.begin() + HEADER_SIZE);
        if (frameHeader != "XZCC") {
            // 帧头不匹配
            return false;
        }
        // 解析数据包类型
        std::string type(buffer.begin() + HEADER_SIZE, buffer.begin() + HEADER_SIZE + TYPE_SIZE_SMALL);
        if (type == "GJ") {
            messageType = type;
            // 跳过2字节类型，开始读取XML数据
            bytesRead = socket.receiveBytes(buffer.begin(), buffer.size());
            if (bytesRead <= 0) {
                // 没有更多数据可读
                return false;
            }
            xmlData.append(buffer.begin(), buffer.begin() + bytesRead);
        } else {
            // 尝试将类型作为整数解析
            try {
                int typeId = std::stoi(type);
                messageType = std::to_string(typeId);
                // 跳过8字节类型，开始读取XML数据
                bytesRead = socket.receiveBytes(buffer.begin(), buffer.size());
                if (bytesRead <= 0) {
                    // 没有更多数据可读
                    return false;
                }
                xmlData.append(buffer.begin(), buffer.begin() + bytesRead);
            } catch (const std::invalid_argument&) {
                // 解析整数失败
                return false;
            }
        }
        // 这里可以添加XML数据的验证逻辑，例如检查是否有完整的XML根元素等
        std::cout<<"xmlData->"<<xmlData<<std::endl; 
        //清空xmlData
        xmlData.clear();    

        return true;
    } catch (Poco::Exception& exc) {
        std::cerr << "DirectorLinkClient::receiveAndParseMessage exception: " << exc.displayText() << std::endl;
        return false;
    }
}

#if 0

#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/NetException.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Element.h>

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

// 函数声明
bool receiveAndParseMessage(StreamSocket& socket, std::string& messageType, std::string& xmlData);

int main() {
    // 初始化日志
    ConsoleChannel* channel = new ConsoleChannel();
    Logger& logger = Logger::create("MyApp", channel);
    logger.setLevel("debug");
    
    // 建立连接
    StreamSocket socket;
    socket.connect(SocketAddress("36.156.64.198", 11011));
    
    // 接收并解析消息
    std::string messageType;
    std::string xmlData;
    if (receiveAndParseMessage(socket, messageType, xmlData)) {
        // 处理解析后的消息
        logger.debug("Message Type: " + messageType);
        logger.debug("XML Data: " + xmlData);
    }
    
    // 关闭连接
    socket.close();
    
    return 0;
}

// 接收并解析消息的实现
bool receiveAndParseMessage(StreamSocket& socket, std::string& messageType, std::string& xmlData) {
    try {
        // 接收固定头
        char header[4];
        int bytesRead = socket.receiveBytes(header, sizeof(header) - 1);
        header[bytesRead] = '\0';
        if (bytesRead != 4 || std::string(header) != "XZCC") {
            return false;
        }
        
        // 接收数据包类型
        char packetType[2];
        bytesRead = socket.receiveBytes(packetType, sizeof(packetType) - 1);
        packetType[bytesRead] = '\0';
        messageType = std::string(packetType);
        
        // 判断数据包类型并接收剩余数据
        if (messageType == "GJ") {
            // 接收XML数据
            std::string xmlBuffer;
            char buffer[1024];
            while (true) {
                bytesRead = socket.receiveBytes(buffer, sizeof(buffer) - 1);
                buffer[bytesRead] = '\0';
                xmlBuffer += buffer;
                
                // 检查是否接收完毕
                if (xmlBuffer.find("</Data>") != std::string::npos) {
                    break;
                }
            }
            xmlData = xmlBuffer;
        } else if (messageType == "ZT") {
            // 处理设备状态数据
        } else if (messageType == "GC") {
            // 处理过车数据
        }
        
        return true;
    } catch (Poco::Exception& e) {
        std::cerr << e.displayText() << std::endl;
        return false;
    }
}

#endif

#if 0
重连机制
void DirectorLinkClient::ReportCarwashInfoWithReconnect(const json &data)
{
    while (!isConnected)
    {
        try
        {
            // 连接到服务器
            if (!_socket->connected())
            {
                _socket->connect(_address);
                isConnected = true;
            }

            // 执行正常的报告流程
            DOMParser parser;
            // ... 你的XML构建代码 ...
            // 创建并发送XML数据
            std::string xmlData = ss.str();
            xmlData = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" + xmlData;
            std::string header = "XZCC";
            std::string packetType = "ZT";
            std::string dataToSend = header + packetType + xmlData;
            _socket->sendBytes(dataToSend.c_str(), dataToSend.length());
        }
        catch (const Poco::Net::ConnectionRefusedException&)
        {
            // 无法连接，等待一段时间后重试
            isConnected = false;
            poco_error_f1(stderr, "Connection refused, will retry in %d ms", reconnectDelay);
            Poco::Thread::sleep(reconnectDelay);
        }
        catch (const Poco::IOException&)
        {
            // 连接中断，等待一段时间后重试
            isConnected = false;
            poco_error_f1(stderr, "IO Exception, will retry in %d ms", reconnectDelay);
            Poco::Thread::sleep(reconnectDelay);
        }

        // 限制最大重试次数
        maxReconnectAttempts--;
        if (maxReconnectAttempts <= 0)
        {
            poco_error(stderr, "Exceeded maximum reconnection attempts, giving up.");
            break;
        }
    }
}

// 在初始化DirectorLinkClient时设置_socket和_address
// 并且可以在适当的地方调用ReportCarwashInfoWithReconnect函数来报告信息

#endif