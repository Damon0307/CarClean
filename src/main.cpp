#include <stdio.h>
#include <iostream>
#include <future>
#include <memory>
#include <fstream>
#include <string>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/route.h> // 包含 struct rtentry 和 RTF_GATEWAY 定义
#include <thread>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <ctime>
#include <sys/time.h>
#include "NetFoundation.h"
#include "WashReport.h"
#include "Timer.h"

#include "DirectorLinkClient.h"
#include "config.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <cstdlib> // 用于setenv
#include <ctime>

// #include "UartMod.h"
using namespace httplib;

const char *RS232_CFG_FILE = "/rs232.json";
const char *NET_CFG_FILE = "/net_cfg.json";
const char *DEF_CFG_FILE = "/default_info.json";

const char *version_str = "RV1106 ntp time,Simple, ip check, AIIPC LOCK ,no exit,25-11-09";

const char *todo_str = " she xiang tou pian yi jian ce";
//const char *version_str = "test update";

std::shared_ptr<spdlog::logger> g_console_logger;
std::shared_ptr<spdlog::logger> g_file_logger;

#if (ARM_FLAG == 1)
const std::string file_path_logger = "/userdata/LogFile.log";
#else
const std::string file_path_logger = "LogFile.log";
#endif

using josn = nlohmann::json;

void SyncTimeFirst();

bool isWithinExitWindow()
{
  // 获取当前时间
  auto now = std::chrono::system_clock::now();
  // 转换为当天的24小时制小时和分钟
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::tm tm_now = *std::localtime(&time_t_now);

  // std::cout<<"now.tm_hour:"<<tm_now.tm_hour<<"now.tm_min:"<<tm_now.tm_min<<std::endl;
  // 检查时间是否在23:25-23:28 之间
  // 这里可以根据需要修改时间范围
  return (tm_now.tm_hour == 23 && tm_now.tm_min >= 29 && tm_now.tm_min <= 31);
}
  
int main()
{
  //一个线程每5分钟同步一次时间 SyncTimeFirst
  // std::thread sync_time_thread([](){
  //   SyncTimeFirst();
  //   this_thread::sleep_for(std::chrono::minutes(2));
  // });
 

  // 设置时区为东八区 (GMT+8)
    setenv("TZ", "CST-8", 1); // 或者使用"GMT-8", 但"CST-8"更常用 
    tzset();  // 应用时区设置
  
    system("killall ntpd");
    //执行 ntpd -p cn.ntp.org.cn -qn
    system("ntpd -q -g -p cn.ntp.org.cn");

 //使用 export TZ=CST-8 命令以后，再用system 命令调用ntpd 命令，就可以同步时间了
  //system("export TZ=CST-8; ntpd -q -g -p time4.tencentyun.com");
  //system("export TZ=CST-8; ntpd -q -g -p ntp.aliyun.com");

  //system("ntpd -q -g -p ntp.aliyun.com");
 

  // 创建一个名为 "console" 的日志对象，输出到标准输出流（彩色）
  g_console_logger = spdlog::stdout_color_mt("console");
  g_console_logger->set_level(spdlog::level::debug); // 设置日志级别
  // 使用日志对象记录日志

  auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      file_path_logger, // 日志文件名
      10 * 1024 * 1024, // 文件最大尺寸：10MB
      3);               // 保留的文件份数

  // 设置日志器名称和sink
  g_file_logger = std::make_shared<spdlog::logger>("file_logger", rotating_sink);

  // 设置日志级别以及其他全局选项
  g_file_logger->set_level(spdlog::level::debug);
  g_file_logger->flush_on(spdlog::level::debug);             // 等于高于debug级别会被立刻刷新到磁盘
  g_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v"); // 设置时间格式等

  // 开始记录日志
  g_console_logger->info("StartUp!!! {}", version_str);
  g_file_logger->info("StartUp!!! {}", version_str);

  std::unique_ptr<NetFoundation> uni_net(new NetFoundation());   // IPC数据接收与数据上传后台处理模块
  std::unique_ptr<WashReport> uni_wash_report(new WashReport()); // 冲洗场景处理模块(包括绕道)

  uni_wash_report.get()->InitDefInfo(DEF_CFG_FILE);
  uni_wash_report.get()->InitSerialComm(RS232_CFG_FILE);
  uni_net.get()->InitNetCFG(NET_CFG_FILE);

  // 同步系统时间
  //uni_net.get()->SyncTimeWithNTP();

  // 绑定冲洗场景的 上传服务器通道
  auto g_post_to_ser_func = std::bind(&NetFoundation::PostDataToServer, uni_net.get(), std::placeholders::_1);
  uni_wash_report.get()->SetPassJsonFunc(g_post_to_ser_func);

  // 绑定冲洗场景的 摄像头数据处理通道
  auto wash_ipc_hander = std::bind(&WashReport::DealWashIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetWashIPCDataHandleFunc(wash_ipc_hander);

  // 绑定车辆进场场景 摄像头数据处理通道
  auto car_in_ipc_hander = std::bind(&WashReport::DealCarInIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetCarInIPCDataHandleFunc(car_in_ipc_hander);

  // 绑定绕道场景的 摄像头数据处理通道
  auto detour_ipc_hander = std::bind(&WashReport::DealDetourIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetDetourIPCDataHandleFunc(detour_ipc_hander);

  // 绑定两侧车轮AI识别干净程度的数据处理通道
  auto wash_l_aiipc_hander = std::bind(&WashReport::Deal_L_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->Set_L_IPCDataHandleFunc(wash_l_aiipc_hander);

  auto wash_r_aiipc_hander = std::bind(&WashReport::Deal_R_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->Set_R_IPCDataHandleFunc(wash_r_aiipc_hander);

  // 传感器数据与摄像头数据处理线程
  std::thread reporter_thread(&WashReport::StartReportingProcess, uni_wash_report.get());

#if 0

  // 每晚退出程序的检测线程 有其他系统脚本实现
  std::thread exit_check_thread([&]()
                                {
    while(1){
        if(isWithinExitWindow()){
            g_console_logger->info("exit check thread exit!");
            g_file_logger->info("exit check thread exit!");
            g_file_logger->flush();
            //当前线程休眠4分钟
            this_thread::sleep_for(chrono::minutes(4));
            exit(0);  
        }
        this_thread::sleep_for(chrono::seconds(60));
    } });
  exit_check_thread.join();

#endif

  uni_net.get()->StartServer();

  reporter_thread.join();

   


  return 0;
}


void SyncTimeFirst()
{
  // // 设置时区为东八区
 
  //先kiil掉之前的ntpd 进程
 // system("killall ntpd");
 
  //设置时区东八区
 // system("export TZ=CST-8");
 // system("export TZ=GMT-8");
 
 // system("ntpd -q -g -p time4.tencentyun.com");

}


#if 0
void SyncTimeFirst()
{
  // 设置时区为东八区
  setenv("TZ", "CST-8", 1); // 或者使用"GMT-8", 但"CST-8"更常用
  tzset();                  // 应用时区更改


// NTP时间戳是从1900年1月1日开始的秒数
#define NTP_TIMESTAMP_DELTA 2208988800ull

// NTP服务器地址
#define NTP_SERVER "36.156.67.46"

// NTP端口
#define NTP_PORT 123

  // NTP消息结构
  struct ntp_packet
  {
    uint8_t li_vn_mode;       // Leap indicator, version and mode
    uint8_t stratum;          // Stratum level
    uint8_t poll;             // Poll interval
    uint8_t precision;        // Precision
    uint32_t root_delay;      // Root delay
    uint32_t root_dispersion; // Root dispersion
    uint32_t ref_id;          // Reference ID
    uint32_t ref_t_sec;       // Reference time-stamp seconds
    uint32_t ref_t_frac;      // Reference time-stamp fraction
    uint32_t orig_t_sec;      // Originate time-stamp seconds
    uint32_t orig_t_frac;     // Originate time-stamp fraction
    uint32_t rx_t_sec;        // Received time-stamp seconds
    uint32_t rx_t_frac;       // Received time-stamp fraction
    uint32_t tx_t_sec;        // Transmit time-stamp seconds
    uint32_t tx_t_frac;       // Transmit time-stamp fraction
  };
  while (1)
  {
    int sockfd;
    struct sockaddr_in serv_addr;
    ntp_packet packet;
    socklen_t len = sizeof(serv_addr);
    struct timeval timeout;

    //休眠100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
    {
      std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
      continue;
    }

    // 设置接收超时
    timeout.tv_sec = 5; // 5秒超时
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
      std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
      close(sockfd);
      continue;
    }

    // 初始化NTP请求包
    memset(&packet, 0, sizeof(ntp_packet));
    packet.li_vn_mode = 0x1B; // LI = 0 (no warning), VN = 3 (IPv4 only), Mode = 3 (Client Mode)

    // 设置NTP服务器地址
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(NTP_SERVER);
    serv_addr.sin_port = htons(NTP_PORT);

    // 发送NTP请求
    if (sendto(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      std::cerr << "Error sending packet: " << strerror(errno) << std::endl;
      close(sockfd);
      continue;
    }

    // 接收NTP响应
    if (recvfrom(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, &len) < 0)
    {
      if (errno == EWOULDBLOCK)
      {
        std::cerr << "NTP request timed out." << std::endl;
      }
      else
      {
        std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
      }
      close(sockfd);
      continue;
    }

    // // 转换时间戳并打印
    time_t tx_time = ntohl(packet.tx_t_sec) - NTP_TIMESTAMP_DELTA;
    struct timeval new_time;
    new_time.tv_sec = tx_time;
    new_time.tv_usec = 0; // NTP时间戳不包含亚秒精度

    if (settimeofday(&new_time, NULL) < 0)
    {
      std::cerr << "Error setting system time: " << strerror(errno) << std::endl;
      close(sockfd);
      continue;
    }

    // 打印新的系统时间
    std::cout << "NTP System time set to: " << ctime(&new_time.tv_sec) << std::endl;
    close(sockfd);
    break;
  }
}
#endif