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

    void DealAIIPCData(const json &pjson)
    {
        try
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (pjson.contains("label") && pjson["label"].is_string())
            {
                const std::string lbl = pjson["label"].get<std::string>();
                // 记录标签队列（仅调试/追踪用途）
                res_queue.push_back(lbl);
                if (res_queue.size() > MAX_AI_LABEL_QUEUE_SIZE)
                {
                    res_queue.pop_front();
                }
                // 标记脏车出现
                if (lbl != "clean")
                {
                    dirty_seen.store(true, std::memory_order_relaxed);
                }
                // 暂存最后一个原始标签（用于区分仅 clean 场景）
                detect_json["raw_label"] = lbl;
            }

            if (pjson.contains("extend") && pjson["extend"].is_object())
            {
                auto &ext = pjson["extend"];
                if (ext.contains("alarm_objs") && ext["alarm_objs"].is_array() && !ext["alarm_objs"].empty())
                {
                    auto &first = ext["alarm_objs"][0];
                    if (first.contains("score") && first["score"].is_number())
                    {
                        detect_json["score"] = first["score"];
                    }
                }
            }

            if (pjson.contains("img_base64") && pjson["img_base64"].is_string())
            {
                const std::string img = pjson["img_base64"].get<std::string>();
                // 记录第一个脏图（只在脏标签首次出现时抓取）
                if (!first_dirty_captured && pjson.contains("label") && pjson["label"].is_string() && pjson["label"].get<std::string>() != "clean")
                {
                    cur_dirty_img = img;
                    first_dirty_captured = true;
                }
                // 始终保存最新图供调试（不覆盖脏图选择链式判断）
                detect_json["latest_img_base64"] = img;
            }

            has_res.store(true, std::memory_order_release);
        }
        catch (const std::exception &e)
        {
            g_console_logger->error("DealAIIPCData exception: {}", e.what());
            g_file_logger->error("DealAIIPCData exception: {}", e.what());
        }
    }

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

private:
    std::atomic<bool> has_res;          // 是否至少收到过一帧
    std::atomic<bool> dirty_seen;       // 是否出现过非 clean 标签
    bool first_dirty_captured;          // 第一张脏图是否已捕获
    json detect_json;                   // 累积的检测元数据
    std::deque<std::string> res_queue;  // 最近标签窗口（调试用途）
    std::string cur_dirty_img;          // 第一张脏图
    std::mutex mtx;                     // 保护聚合状态
};

#endif // __AIIPC_H__