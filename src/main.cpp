#include <stdio.h>
#include <iostream>
#include <memory>
#include "NetFoundation.h"
#include "WashReport.h"
#include "Timer.h"
#include "DirectorLinkClient.h"
#include "config.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// #include "UartMod.h"
using namespace httplib;

const char *RS232_CFG_FILE = "rs232.json";
const char *NET_CFG_FILE = "net_cfg.json";
const char *DEF_CFG_FILE = "default_info.json";
const char *DIRECT_LINK_CFG_FILE = "direct_link.json";

const char *version_str = "Version  单雷达  AI 2s relay 闸机 0817 +清AI 缓存";

std::shared_ptr<spdlog::logger> g_console_logger;
std::shared_ptr<spdlog::logger> g_file_logger;
 
#if(ARM_FLAG==1)
const std::string file_path_logger = "/userdata/LogFile.log";
#else
const std::string file_path_logger = "LogFile.log";
#endif
 

using josn = nlohmann::json;

bool isWithinExitWindow() {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    // 转换为当天的24小时制小时和分钟
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_t_now);
    
   // std::cout<<"now.tm_hour:"<<tm_now.tm_hour<<"now.tm_min:"<<tm_now.tm_min<<std::endl;
    // 检查时间是否在23:25-23:28 之间
    // 这里可以根据需要修改时间范围
    return (tm_now.tm_hour == 23 && tm_now.tm_min >= 25 && tm_now.tm_min <= 28);
}





int main() 
{


#if 1
 
    // 创建一个名为 "console" 的日志对象，输出到标准输出流（彩色）
     g_console_logger = spdlog::stdout_color_mt("console");
     g_console_logger->set_level(spdlog::level::debug); // 设置日志级别
    // 使用日志对象记录日志

    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
         file_path_logger,  // 日志文件名
        10 * 1024 * 1024,       // 文件最大尺寸：10MB
        3);                     // 保留的文件份数

    // 设置日志器名称和sink
    g_file_logger = std::make_shared<spdlog::logger>("file_logger", rotating_sink);

    // 设置日志级别以及其他全局选项
    g_file_logger->set_level(spdlog::level::debug);
    g_file_logger->flush_on(spdlog::level::debug);//等于高于debug级别会被立刻刷新到磁盘
    g_file_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");// 设置时间格式等

    // 开始记录日志
    g_console_logger->info("StartUp!!! {}", version_str); 
    g_file_logger->info("StartUp!!! {}", version_str);
 
 
#if 1
 
  std::unique_ptr<NetFoundation> uni_ccr(new NetFoundation());  //IPC数据接收与数据上传后台处理模块
  std::unique_ptr<WashReport> uni_wash_report(new WashReport());  //冲洗场景处理模块(包括绕道)
  
  
  #if(DIRECTOR_LINK_ENABLE==1)
  std::unique_ptr<DirectorLinkClient> uni_dl_client(new DirectorLinkClient(DIRECT_LINK_CFG_FILE));
  auto dl_report_wash_func = std::bind(&DirectorLinkClient::ReportCarWashInfo, uni_dl_client.get(), std::placeholders::_1,std::placeholders::_2); 
  auto dl_car_pass_func = std::bind(&DirectorLinkClient::ReportCarPass, uni_dl_client.get(), std::placeholders::_1,std::placeholders::_2);
  auto dl_report_status_func = std::bind(&DirectorLinkClient::ReportStatus, uni_dl_client.get(), std::placeholders::_1,std::placeholders::_2);  
  
  uni_wash_report.get()->SetDLWashFunc(dl_report_wash_func);
  uni_wash_report.get()->SetDLCarPassFunc(dl_car_pass_func);
  uni_wash_report.get()->SetDLStatusFunc(dl_report_status_func);  
  #endif


  uni_wash_report.get()->InitDefInfo(DEF_CFG_FILE);
  uni_wash_report.get()->InitSerialComm(RS232_CFG_FILE);
  uni_ccr.get()->InitNetCFG(NET_CFG_FILE);
  

 //绑定冲洗场景的 上传服务器通道
  auto g_post_to_ser_func = std::bind(&NetFoundation::PostDataToServer, uni_ccr.get(), std::placeholders::_1);
  uni_wash_report.get()->SetPassJsonFunc(g_post_to_ser_func);

 //绑定冲洗场景的 摄像头数据处理通道
  auto wash_ipc_hander=std::bind(&WashReport::DealWashIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->SetWashIPCDataHandleFunc(wash_ipc_hander);

  //绑定车辆进场场景 摄像头数据处理通道
  auto car_in_ipc_hander=std::bind(&WashReport::DealCarInIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->SetCarInIPCDataHandleFunc(car_in_ipc_hander); 

 //绑定绕道场景的 摄像头数据处理通道
  auto detour_ipc_hander=std::bind(&WashReport::DealDetourIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->SetDetourIPCDataHandleFunc(detour_ipc_hander);

//绑定两侧车轮AI识别干净程度的数据处理通道
  auto wash_l_aiipc_hander=std::bind(&WashReport::Deal_L_AIIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->Set_L_IPCDataHandleFunc(wash_l_aiipc_hander);

  auto wash_r_aiipc_hander=std::bind(&WashReport::Deal_R_AIIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->Set_R_IPCDataHandleFunc(wash_r_aiipc_hander);
 
  //传感器数据与摄像头数据处理线程
  std::thread reporter_thread(&WashReport::StartReportingProcess,uni_wash_report.get());
  
  //每晚退出程序的检测线程
  std::thread exit_check_thread([&](){
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
    }
  }); 
 
#if(DIRECTOR_LINK_ENABLE==1)
  //直连模块接收服务端消息线程
  std::thread dl_client_thread([&uni_dl_client](){
       uni_dl_client.get()->receiveAndParseMessage();  
  });
#endif
  
  uni_ccr.get()->StartServer();
 
  reporter_thread.join();
 #endif 
 
 #if(DIRECTOR_LINK_ENABLE==1)
  dl_client_thread.join();  
#endif

  exit_check_thread.join();
  
  return 0;

 #endif 
}
