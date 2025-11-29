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
        //only for roof camera
        roof_uncovered_seen.store(false, std::memory_order_relaxed);
        first_uncovered_captured = false;
        roof_detect_json = {};
        roof_res_queue.clear();
        roof_cur_uncovered_img.clear();
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

    json GetRoofDetectRes()
    {
        std::lock_guard<std::mutex> lk(mtx);
        json out = roof_detect_json; // 拷贝当前快照
        // 决策 label
        if (roof_uncovered_seen.load(std::memory_order_relaxed))
        {
            out["label"] = "uncovered";
            if (!roof_cur_uncovered_img.empty())
            {
                out["img_base64"] = roof_cur_uncovered_img; // 第一张 uncovered 图
            }
        }
        else if (has_res.load(std::memory_order_relaxed))
        {
            // 只有出现过数据但没有 uncovered 标签时判断为 covered
            out["label"] = "covered";
            // 若未出现 uncovered，提供最新图片（如果有）
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
      
        std::lock_guard<std::mutex> lk(mtx);
        if (aiipc_type == 1)
        {
            return has_res.load(std::memory_order_acquire);
        }else{
            //复合摄像头需要判断两个队列是否都有数据
            if(roof_res_queue.size() > 0 && res_queue.size() > 0){
                return true;
            }else{
                return false;
            }
        }
        return false;
    }
 
    bool IsCovered()
    {
        std::lock_guard<std::mutex> lk(mtx);
        if (aiipc_type == 3)
        {
            return !roof_uncovered_seen.load(std::memory_order_relaxed);
        }
        return false;
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
    json detect_json;                   // 聚合结果

    json roof_detect_json;               // 顶棚聚合结果

    std::deque<std::string> res_queue;  // 最近标签窗口（调试用途）
    std::string cur_dirty_img;          // 第一张脏图
    std::mutex mtx;                     // 保护聚合状态


    //only for side + roof camera
    std::deque<std::string> roof_res_queue;  // 最近标签窗口（调试用途）
    std::atomic<bool> roof_uncovered_seen; // 是否出现过 uncovered 标签（顶棚专用
    std::string roof_cur_uncovered_img;          // 第一张uncovered图
    bool first_uncovered_captured;          // 第一张uncovered图是否已捕获


    void DealWheelAIIPCData(const json &pjson);
    void DealBodyAIIPCData(const json &pjson);
    void DealRoofAIIPCData(const json &pjson);


};

#endif // __AIIPC_H__