#ifndef __AIIPC_H__
#define __AIIPC_H__

/**
 * AI 摄像头模块抽象（简化版）
 *
 * 所有摄像机统一处理 clean/dirty 标签 + score + img_base64。
 * 通过 aiipc_type 区分摄像机用途（仅影响日志/调试）。
 *
 * 当前使用的摄像机:
 *   type 1: 左轮 / 右轮
 *   type 3: 车尾
 *   (type 2: 车身侧面 — 暂时屏蔽)
 */

#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <memory>
#include <atomic>
#include "json.hpp"
#include "spdlog/spdlog.h"

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

#define MAX_AI_LABEL_QUEUE_SIZE 25

using json = nlohmann::json;
using namespace std;

class AIIPC
{
public:
    enum AIIPCType {
        TYPE_WHEEL = 1,       // 车轮
        TYPE_SIDE_BODY = 2,   // 车身侧面（暂时屏蔽）
        TYPE_TAIL = 3         // 车尾
    };

    AIIPC() : aiipc_type(TYPE_WHEEL), has_res(false), dirty_seen(false),
              first_dirty_captured(false) {}
    ~AIIPC() = default;

    void SetAIIPCType(int t) { aiipc_type = t; }

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

    // 统一入口：所有类型处理 clean/dirty 标签
    void DealAIIPCData(const json &pjson)
    {
        try
        {
            std::lock_guard<std::mutex> lk(mtx);

            if (!pjson.contains("label") || !pjson["label"].is_string())
                return;

            const std::string lbl = pjson["label"].get<std::string>();

            // 仅处理 clean / dirty 标签
            if (lbl != "clean" && lbl != "dirty")
                return;

            // 记录标签队列
            res_queue.push_back(lbl);
            if (res_queue.size() > MAX_AI_LABEL_QUEUE_SIZE)
                res_queue.pop_front();

            // 标记脏车
            if (lbl == "dirty")
                dirty_seen.store(true, std::memory_order_relaxed);

            detect_json["raw_label"] = lbl;

            // 提取 score
            if (pjson.contains("extend") && pjson["extend"].is_object())
            {
                auto &ext = pjson["extend"];
                if (ext.contains("alarm_objs") && ext["alarm_objs"].is_array() && !ext["alarm_objs"].empty())
                {
                    auto &first = ext["alarm_objs"][0];
                    if (first.contains("score") && first["score"].is_number())
                        detect_json["score"] = first["score"];
                }
            }

            // 提取图片
            if (pjson.contains("img_base64") && pjson["img_base64"].is_string())
            {
                const std::string img = pjson["img_base64"].get<std::string>();
                if (!first_dirty_captured && lbl == "dirty")
                {
                    cur_dirty_img = img;
                    first_dirty_captured = true;
                }
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

    // 获取检测结果
    json GetDetectRes()
    {
        std::lock_guard<std::mutex> lk(mtx);
        json out = detect_json;
        if (dirty_seen.load(std::memory_order_relaxed))
        {
            out["label"] = "dirty";
            if (!cur_dirty_img.empty())
                out["img_base64"] = cur_dirty_img;
        }
        else if (has_res.load(std::memory_order_relaxed))
        {
            out["label"] = "clean";
            if (out.contains("latest_img_base64"))
                out["img_base64"] = out["latest_img_base64"];
        }
        else
        {
            out["label"] = "unknown";
        }
        return out;
    }

    bool GetResult() const { return has_res.load(std::memory_order_acquire); }

private:
    int aiipc_type;
    std::atomic<bool> has_res;
    std::atomic<bool> dirty_seen;
    bool first_dirty_captured;
    json detect_json;
    std::deque<std::string> res_queue;
    std::string cur_dirty_img;
    std::mutex mtx;
};

#endif // __AIIPC_H__
