#ifndef __NETFOUNDATION_H__
#define __NETFOUNDATION_H__


/**
 * @brief  网络连接相关功能处理
 * @author Damon0307
 * @copyright 1294056177@qq.com
 * @date 2023/09/16
*/


#include <string> 
#include "httplib.h"
#include "json.hpp"
 
using json =nlohmann::json;
using namespace httplib;

class NetFoundation
{

public:
    NetFoundation(/* args */);
    ~NetFoundation();
 
    void InitNetCFG(const char* file_name);
    //处理 正常冲洗场景的  摄像头数据
    void WashIPCDataHandler(const Request& req, Response& res);
    void DetourIPCDataHandler(const Request& req, Response& res);
    void CarInIPCDataHandler(const Request& req, Response& res);
    void LeftSideAIIPCDataHandler(const Request& req, Response& res);
    void RightSideAIIPCDataHandler(const Request& req, Response& res);


 
    void PostDataToServer(json p_json);
    
    void SetWashIPCDataHandleFunc(std::function<void(const json &, Response&)> p_func)
    {
      wash_hadler_func =  p_func; 
    }

    void SetDetourIPCDataHandleFunc(std::function<void(const json &, Response&)> p_func)
    {
      detour_hadler_func =  p_func; 
    }

    void Set_L_IPCDataHandleFunc(std::function<void(const json &, Response&)> p_func)
    {
      wash_l_aiipc_func =  p_func; 
    }

    void Set_R_IPCDataHandleFunc(std::function<void(const json &, Response&)> p_func)
    {
      wash_r_aiipc_func =  p_func; 
    }
    void SetCarInIPCDataHandleFunc(std::function<void(const json &, Response&)> p_func)
    {
      car_in_ipc_func =  p_func; 
    } 
 
    //注册AIIPC 的webhook
    void RegisterWebHookForAIIPC();

//服务器开始监听
    void StartServer();
    
    void ConfigRV1106IP(const std::string& ip);

    //ntp时间同步
    void SyncTimeWithNTP();

private:
// json参考链接  https://www.cnblogs.com/linuxAndMcu/p/14503341.html
    
    //冲洗摄像头
    std::function<void(const json&, Response&)> wash_hadler_func;
    //绕道摄像头
    std::function<void(const json&, Response&)> detour_hadler_func;
    //左侧ai ipc
    std::function<void(const json&, Response&)> wash_l_aiipc_func;
    //右侧ai ipc
    std::function<void(const json&, Response&)> wash_r_aiipc_func;
    //车辆入场摄像头
    std::function<void(const json&, Response&)> car_in_ipc_func; 
  
    httplib::Server mServer;

    std::string local_server;
    std::string remote_server;
    int local_port;
    int remote_port;
    std::string GetPhyIP(const std::string& interface);

    std::thread ip_check_thread;

};


 
 #endif // __NETFOUNDATION_H__