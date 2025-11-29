#include "AIIPC.h"

// 现在需要根据不同类型摄像机做不同处理
void AIIPC::DealAIIPCData(const json &pjson)
{
    try
    {
        std::lock_guard<std::mutex> lk(mtx);
        if (aiipc_type == 1 || aiipc_type == 2)
        {
            DealBodyAIIPCData(pjson);
        }
        else if (aiipc_type == 3)
        {
            DealRoofAIIPCData(pjson);
        }
    }
    catch (const std::exception &e)
    {
        g_console_logger->error("DealAIIPCData exception: {}", e.what());
        g_file_logger->error("DealAIIPCData exception: {}", e.what());
    }
}

void AIIPC::DealWheelAIIPCData(const json &pjson)
{
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

// body 和 wheel 只处理 clean 和 dirty 标签，其他标签忽略

void AIIPC::DealBodyAIIPCData(const json &pjson)
{
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
        if (lbl == "dirty")
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
        if (!first_dirty_captured && pjson.contains("label") && pjson["label"].is_string() && pjson["label"].get<std::string>() == "dirty")
        {
            cur_dirty_img = img;
            first_dirty_captured = true;
        }
        // 始终保存最新图供调试（不覆盖脏图选择链式判断）
        detect_json["latest_img_base64"] = img;
    }
    has_res.store(true, std::memory_order_release);
}

// 顶棚摄像头会有两种标签，当出现 clean,dirty 时候代表此次上报的是车身干净与否
// 当出现 uncovered,covered 时候代表此次上报的是车顶是否有顶棚

void AIIPC::DealRoofAIIPCData(const json &pjson)
{
    // 分类处理标签，组后由各自的队列中数据返回结果
    if (pjson.contains("label") && pjson["label"].is_string())
    {
        const std::string lbl = pjson["label"].get<std::string>();
        // 如果标签中存储的是 clean dirty 则处理车身干净与否，否则处理车顶是否有顶棚
        if (lbl == "clean" || lbl == "dirty")
        {
            // 记录标签队列（仅调试/追踪用途）
            res_queue.push_back(lbl);
            if (res_queue.size() > MAX_AI_LABEL_QUEUE_SIZE)
            {
                res_queue.pop_front();
            }
            // 标记脏车出现
            if (lbl == "dirty")
            {
                dirty_seen.store(true, std::memory_order_relaxed);
            }
            // 处理第一个脏图
            if (pjson.contains("img_base64") && pjson["img_base64"].is_string())
            {
                const std::string img = pjson["img_base64"].get<std::string>();
                // 记录第一个脏图（只在脏标签首次出现时抓取）
                if (!first_dirty_captured && lbl == "dirty")
                {
                    cur_dirty_img = img;
                    first_dirty_captured = true;
                }
                // 始终保存最新图供调试（不覆盖脏图选择链式判断）
                detect_json["latest_img_base64"] = img;
            }
            // 暂存最后一个原始标签（用于区分仅 clean 场景
            detect_json["raw_label"] = lbl;
            // 处理extend部分分数
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
        }
        else if (lbl == "uncovered" || lbl == "covered")
        {
            // 记录标签队列（仅调试/追踪用途）
            roof_res_queue.push_back(lbl);
            if (roof_res_queue.size() > MAX_AI_LABEL_QUEUE_SIZE)
            {
                roof_res_queue.pop_front();
            }
            // 标记 uncovered 出现
            if (lbl == "uncovered")
            {
                roof_uncovered_seen.store(true, std::memory_order_relaxed);
            }
            // 暂存最后一个原始标签（用于区分仅 covered 场景）
            roof_detect_json["raw_label"] = lbl;
            // 处理第一个 uncovered 图
            if (pjson.contains("img_base64") && pjson["img_base64"].is_string())
            {
                const std::string img = pjson["img_base64"].get<std::string>();
                // 记录第一个脏图（只在脏标签首次出现时抓取）
                if (!first_uncovered_captured && lbl == "uncovered")
                {
                    roof_cur_uncovered_img = img;
                    first_uncovered_captured = true;
                }
                // 始终保存最新图供调试（不覆盖脏图选择链式判断）
                roof_detect_json["latest_img_base64"] = img;
            }
            // 处理extend部分分数
            if (pjson.contains("extend") && pjson["extend"].is_object())
            {
                auto &ext = pjson["extend"];
                if (ext.contains("alarm_objs") && ext["alarm_objs"].is_array() && !ext["alarm_objs"].empty())
                {
                    auto &first = ext["alarm_objs"][0];
                    if (first.contains("score") && first["score"].is_number())
                    {
                        roof_detect_json["score"] = first["score"];
                    }
                }
            }
        }
    }
}

/**
 *
 * 示例JSON
  {
  "device_id": "((DEVICE_ID))",
  "device_version": "((DEVICE_VERSION))",
  "date": "((DATE_FORMAT))",
  "timestamp": ((TIMESTAMP)),
  "label": "((LABEL))",
  "alias": "((ALIAS))",
  "count": ((COUNT)),
  "img_base64": "((BASE64_IMAGE))",
  "extend": {
    ((EXTEND))
  }
}
}

 */