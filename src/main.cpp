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

const char *version_str = "RV1106 ntp time,Simple, ip check, AIIPC LOCK,add RLTR 4aiipc ,no exit,25-11-25";

//接入6个ai ipc

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
      50 * 1024 * 1024, // 文件最大尺寸：50MB
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

  // 绑定车尾AI识别干净程度的数据处理通道
  auto wash_tail_aiipc_hander = std::bind(&WashReport::Deal_Tail_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetTailIPCDataHandleFunc(wash_tail_aiipc_hander);

  // 绑定车顶AI识别干净程度的数据处理通道
  auto wash_roof_aiipc_hander = std::bind(&WashReport::Deal_Roof_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetRoofIPCDataHandleFunc(wash_roof_aiipc_hander);

  // 绑定左侧AI识别干净程度的数据处理通道
  auto wash_side_l_aiipc_hander = std::bind(&WashReport::Deal_Side_L_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetLeftSideIPCDataHandleFunc(wash_side_l_aiipc_hander);

  // 绑定右侧AI识别干净程度的数据处理通道
  auto wash_side_r_aiipc_hander = std::bind(&WashReport::Deal_Side_R_AIIPCData, uni_wash_report.get(), std::placeholders::_1, std::placeholders::_2);
  uni_net.get()->SetRightSideIPCDataHandleFunc(wash_side_r_aiipc_hander);
 

  // 传感器数据与摄像头数据处理线程
  std::thread reporter_thread(&WashReport::StartReportingProcess, uni_wash_report.get());

#if 0

  // 每晚退出程序的检测线程 有其他系统脚本实现
  std::thread exit_check_thread([&]()
                                {
    while(1){
        if(isWithinExitWindow())
        {
     
            //当前线程休眠4分钟
            this_thread::sleep_for(chrono::minutes(2));
               //执行系统重启命令
          g_console_logger->info("system reboot now!");
          g_file_logger->info("system reboot now!");
          g_file_logger->flush();
          system("reboot");
          exit(0);  
        }
        this_thread::sleep_for(chrono::seconds(10));
    } });


#endif

  uni_net.get()->StartServer();

  reporter_thread.join();

  //exit_check_thread.join();


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


