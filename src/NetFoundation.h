#ifndef __NETFOUNDATION_H__
#define __NETFOUNDATION_H__

/**
 * @brief  网络连接相关功能处理
 *
 * 路由映射:
 *   /wash_report      → WashIPCDataHandler      (车牌抓拍)
 *   /detour_report     → DetourIPCDataHandler     (绕道抓拍)
 *   /car_in            → CarInIPCDataHandler      (车辆进场)
 *   /aiipc/left        → LeftWheelAIIPCHandler    (左轮AI)
 *   /aiipc/right       → RightWheelAIIPCHandler   (右轮AI)
 *   /aiipc/roof_and_tail → RoofAIIPCHandler       (顶棚+车尾AI)
 *   /aiipc/side_left   → LeftSideAIIPCHandler     (左侧车身AI)
 *   /aiipc/side_right  → RightSideAIIPCHandler    (右侧车身AI)
 */

#include <string>
#include <map>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

using IPCHandlerFunc = std::function<void(const json&, Response&)>;

class NetFoundation
{
public:
    NetFoundation(/* args */);
    ~NetFoundation();

    void InitNetCFG(const char* file_name);

    // IPC 数据处理handler（thin wrappers → 内部委托给GenericHandler）
    void WashIPCDataHandler(const Request& req, Response& res);
    void DetourIPCDataHandler(const Request& req, Response& res);
    void CarInIPCDataHandler(const Request& req, Response& res);
    void LeftWheelAIIPCHandler(const Request& req, Response& res);
    void RightWheelAIIPCHandler(const Request& req, Response& res);
    void TailAIIPCHandler(const Request& req, Response& res);
    void RoofAIIPCHandler(const Request& req, Response& res);
    void LeftSideAIIPCHandler(const Request& req, Response& res);
    void RightSideAIIPCHandler(const Request& req, Response& res);

    bool PostDataToServer(json p_json);

    // 设置各IPC数据处理的回调函数
    void SetWashIPCDataHandleFunc(IPCHandlerFunc func)    { m_handlers[WASH_IPC] = func; }
    void SetDetourIPCDataHandleFunc(IPCHandlerFunc func)  { m_handlers[DETOUR_IPC] = func; }
    void Set_L_IPCDataHandleFunc(IPCHandlerFunc func)     { m_handlers[LEFT_WHEEL_AI] = func; }
    void Set_R_IPCDataHandleFunc(IPCHandlerFunc func)     { m_handlers[RIGHT_WHEEL_AI] = func; }
    void SetCarInIPCDataHandleFunc(IPCHandlerFunc func)   { m_handlers[CAR_IN_IPC] = func; }
    void SetTailIPCDataHandleFunc(IPCHandlerFunc func)    { m_handlers[TAIL_AI] = func; }
    void SetRoofIPCDataHandleFunc(IPCHandlerFunc func)    { m_handlers[ROOF_AI] = func; }
    void SetLeftSideIPCDataHandleFunc(IPCHandlerFunc func)  { m_handlers[LEFT_SIDE_AI] = func; }
    void SetRightSideIPCDataHandleFunc(IPCHandlerFunc func) { m_handlers[RIGHT_SIDE_AI] = func; }

    void StartServer();
    void ConfigRV1106IP(const std::string& ip);
    void SyncTimeWithNTP();

private:
    // IPC handler keys
    enum HandlerKey {
        WASH_IPC,
        DETOUR_IPC,
        CAR_IN_IPC,
        LEFT_WHEEL_AI,
        RIGHT_WHEEL_AI,
        TAIL_AI,
        ROOF_AI,
        LEFT_SIDE_AI,
        RIGHT_SIDE_AI
    };

    // 通用处理：解析JSON并委托给对应handler
    void GenericHandler(HandlerKey key, const Request& req, Response& res)
    {
        auto body = req.body;
        json req_data = json::parse(body);
        auto it = m_handlers.find(key);
        if (it != m_handlers.end() && it->second)
        {
            it->second(req_data, res);
        }
    }

    std::map<HandlerKey, IPCHandlerFunc> m_handlers;

    httplib::Server mServer;

    std::string local_server;
    std::string remote_server;
    int local_port;
    int remote_port;
    std::string GetPhyIP(const std::string& interface);

    std::thread ip_check_thread;
};

#endif // __NETFOUNDATION_H__
