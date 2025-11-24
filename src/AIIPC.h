#ifndef __AIIPC_H__
#define __AIIPC_H__

/**
 * AI 摄像头模块抽象
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <deque>
#include <mutex>
#include <memory>
#include <atomic>
#include "json.hpp"
#include "spdlog/spdlog.h" // 引入spdlog主头文件，包含logger定义

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

#define MAX_AI_LABEL_QUEUE_SIZE 25

using json = nlohmann::json;
using namespace std;

//2025-11-24 新增摄像机种类 1 车轮，2 车身面 3 车顶棚
enum class CameraType {
    WHEEL = 1,
    BODY = 2,
    ROOF = 3
};

class AIIPC
{
public:
    AIIPC() : has_res(false), dirty_seen(false), first_dirty_captured(false) {}
    ~AIIPC() = default;

    void ResetStatus()
    {
        std::lock_guard<std::mutex> lk(mtx);
        has_res.store(false, std::memory_order_relaxed);
        dirty_seen.store(false, std::memory_order_relaxed);
        first_dirty_captured = false;
        detect_json = {};
        res_queue.clear();
        cur_dirty_img.clear();
    }

    void DealAIIPCData(const json &pjson);
  
    json GetDetectRes()
    {
        std::lock_guard<std::mutex> lk(mtx);
        json out = detect_json; // 拷贝当前快照
        // 决策 label
        if (dirty_seen.load(std::memory_order_relaxed))
        {
            out["label"] = "dirty";
            if (!cur_dirty_img.empty())
            {
                out["img_base64"] = cur_dirty_img; // 第一张脏图
            }
        }
        else if (has_res.load(std::memory_order_relaxed))
        {
            // 只有出现过数据但没有脏标签时判断为 clean
            out["label"] = "clean";
            // 若未出现脏，提供最新图片（如果有）
            if (out.contains("latest_img_base64"))
            {
                out["img_base64"] = out["latest_img_base64"];
            }
        }
        else
        {
            out["label"] = "unknown";
        }
        return out;
    }

    bool GetResult()
    {
        return has_res.load(std::memory_order_acquire);
    }
 

    void SetAIIPCType(int type)
    {
        aiipc_type = type;
    }

private:
    int aiipc_type = 1;                     // AI IPC 类型标识
    std::atomic<bool> has_res;          // 是否至少收到过一帧
    std::atomic<bool> dirty_seen;       // 是否出现过非 clean 标签
    bool first_dirty_captured;          // 第一张脏图是否已捕获
    json detect_json;                   // 累积的检测元数据
    std::deque<std::string> res_queue;  // 最近标签窗口（调试用途）
    std::string cur_dirty_img;          // 第一张脏图
    std::mutex mtx;                     // 保护聚合状态

    void DealWheelAIIPCData(const json &pjson);
    void DealBodyAIIPCData(const json &pjson);
    void DealRoofAIIPCData(const json &pjson);


};

#endif // __AIIPC_H__