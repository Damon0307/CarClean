#include <stdio.h>
#include <iostream>
#include <memory>

#include "NetFoundation.h"
#include "WashReport.h"
 

#include "Timer.h"

// #include "UartMod.h"
using namespace httplib;

const char *RS232_CFG_FILE = "rs232.json";
const char *NET_CFG_FILE = "net_cfg.json";
const char *DEF_CFG_FILE = "default_info.json";

 
int main()
{
  printf("CarClean Debug ver 1.4 start up \n");
 
 
  // std::unique_ptr<UartMod> uni_uart(new UartMod());  //串口模块
  std::unique_ptr<NetFoundation> uni_ccr(new NetFoundation());  //IPC数据接收与数据上传后台处理模块
  std::unique_ptr<WashReport> uni_wash_report(new WashReport());  //冲洗场景处理模块(包括绕道)
 

  uni_wash_report.get()->InitDefInfo(DEF_CFG_FILE);
  uni_wash_report.get()->InitSerialComm(RS232_CFG_FILE);
  uni_ccr.get()->InitNetCFG(NET_CFG_FILE);
  
#if 1
 //绑定冲洗场景的 上传服务器通道
  auto g_post_to_ser_func = std::bind(&NetFoundation::PostDataToServer, uni_ccr.get(), std::placeholders::_1);
  uni_wash_report.get()->SetPassJsonFunc(g_post_to_ser_func);

 //绑定冲洗场景的 摄像头数据处理通道
  auto wash_ipc_hander=std::bind(&WashReport::DealWashIPCData,uni_wash_report.get(),std::placeholders::_1,std::placeholders::_2);
  uni_ccr.get()->SetWashIPCDataHandleFunc(wash_ipc_hander);


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
 
  uni_ccr.get()->StartServer();
 
  reporter_thread.join();


#endif 
 
  return 0;
}

//todo 离开时间改成点的方式， 对比上一次上报的车牌
 
