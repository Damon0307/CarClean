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
#include "json.hpp"

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;


using json = nlohmann::json;
using namespace std;

class AIIPC
{

public:
    AIIPC(/* args */)
    {
        has_res = false;
    }
    ~AIIPC()
    {
    }
    void ResetStatus()
    {
        has_res = false;
        detect_json = {};
        std::deque<std::string> empty_queue = {};
        res_queue.swap(empty_queue);
        cur_dirty_img="";
    }

    void DealAIIPCData(const json &pjson)
    {   
       // g_file_logger->debug("AI IPC data: {}", pjson.dump());   // 记录AI IPC数据
        if (pjson.contains("label"))
        {
            res_queue.push_back(pjson["label"]);
        }

        if (pjson.contains("label"))
            detect_json["label"] = pjson["label"];
        if (pjson.contains("img_base64") ==true)    
        {//记录当前第一次dirty的照片
            if(cur_dirty_img=="" && pjson["label"]!="clean")
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

        has_res = true;
    }

    json GetDetectRes()
    {
        for (auto &i : res_queue)
        {
            if (i != "clean")
            {
                detect_json["label"] = "dirty";
                detect_json["img_base64"] = cur_dirty_img;  
            }
        }
        return detect_json;
    }

    bool GetResult()
    {
        return has_res;
    }

private:
    bool has_res;
    json detect_json;
    std::deque<std::string> res_queue;
    std::string cur_dirty_img;
};

#endif // __AIIPC_H__