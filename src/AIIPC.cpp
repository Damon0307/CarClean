#include "AIIPC.h"

#define MAX_AIIPC_DATA_SIZE 50 

void AIIPC::DealAIIPCData(const json &pjson)
{
    std::lock_guard<std::mutex> lock(mutex_aiipc_data);

    if (pjson.contains("label"))
    {
        // 记录当前时间并存入队列
        time_t cur_time;
        time(&cur_time);
        AIIPC_Data aiipc_data;
        aiipc_data.timestamp = cur_time;
        detect_json["label"] = pjson["label"];
        if (pjson.contains("img_base64") == true)
        { // 记录当前第一次dirty的照片
            if (cur_dirty_img == "" && pjson["label"] != "clean")
            {
                cur_dirty_img = pjson["img_base64"];
            }
            detect_json["img_base64"] = pjson["img_base64"];
        }
        if (pjson.contains("device_id"))
            detect_json["device_id"] = pjson["device_id"];
        if (pjson.contains("device_version"))
            detect_json["device_version"] = pjson["device_version"];
        if (pjson.contains("date"))
            detect_json["date"] = pjson["date"];
        if (pjson.contains("timestamp"))
            detect_json["timestamp"] = pjson["timestamp"];
        if (pjson.contains("extend") && pjson["extend"].contains("alarm_objs") && pjson["extend"]["alarm_objs"].size() > 0 && pjson["extend"]["alarm_objs"][0].contains("score"))
            detect_json["score"] = pjson["extend"]["alarm_objs"][0]["score"];

        aiipc_data.res_json = detect_json; // AIIPC_Data 结构体包含 aiipc 的 json 结果以及当前时间

        if (aiipc_data_list.size() >= MAX_AIIPC_DATA_SIZE)
        {
            aiipc_data_list.pop_front();
        }   
        aiipc_data_list.push_back(aiipc_data);
        has_res = true; // 检测到有结果 
    }
    else
    {
        g_console_logger->warn("AI IPC data not contains label");
        g_file_logger->warn("AI IPC data not contains label");
        return; // 没有label,则直接 return
    }
}

// 获取并处理指定时间段内的AI IPC数据  time_interval 参数 单位是秒
json AIIPC::GetDetectRes(int time_interval)
{
    std::lock_guard<std::mutex> lock(mutex_aiipc_data);
    
    time_t cur_time = time(NULL);
    std::deque<AIIPC_Data> tmp_data_list;
    for (auto it = aiipc_data_list.begin(); it != aiipc_data_list.end(); ++it)
    {
        if (difftime(cur_time, it->timestamp) < time_interval)
            tmp_data_list.push_back(*it);
    }
    // 检查该时间段的JSON中是不是都是干净
    for (auto i : tmp_data_list)
    {
        if (i.res_json["label"] != "clean")
        {
            return i.res_json; // 如果有不干净，那么返回这个JSON
        }
    }
    // 运行到这里说明所有的JSON都是干净，返回第一个干净的JSON
    if (tmp_data_list.size() > 0)
    {
        return tmp_data_list.front().res_json;
    }
    else
    {
        g_file_logger->warn("No detected AI IPC data");
        g_console_logger->warn("No detected AI IPC data");
    }
    return json();  // 返回空的JSON 
}
