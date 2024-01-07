#ifndef __AIIPC_H__
#define __AIIPC_H__

/**
 * AI 摄像头模块抽象
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "json.hpp"

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
    }

    void DealAIIPCData(const json &pjson)
    {
        // detect_json["label"] = pjson["label"];
        // detect_json["img_base64"] = pjson["img_base64"];
        // detect_json["device_id"] = pjson["device_id"];
        // detect_json["device_version"] = pjson["device_version"];
        // detect_json["date"] = pjson["date"];
        // detect_json["timestamp"] = pjson["timestamp"];
        // detect_json["score"] = pjson["extend"]["alarm_objs"][0]["score"];

 if (pjson.contains("label")) detect_json["label"] = pjson["label"];
 if (pjson.contains("img_base64")) detect_json["img_base64"] = pjson["img_base64"];
 if (pjson.contains("device_id")) detect_json["device_id"] = pjson["device_id"];
 if (pjson.contains("device_version")) detect_json["device_version"] = pjson["device_version"];
 if (pjson.contains("date")) detect_json["date"] = pjson["date"];
 if (pjson.contains("timestamp")) detect_json["timestamp"] = pjson["timestamp"];
 if (pjson.contains("extend") && pjson["extend"].contains("alarm_objs") && pjson["extend"]["alarm_objs"].size() > 0 && pjson["extend"]["alarm_objs"][0].contains("score"))
     detect_json["score"] = pjson["extend"]["alarm_objs"][0]["score"];


        has_res = true;
    }

    json GetDetectRes()
    {
        return detect_json;
    }

    bool GetResult()
    {
       return has_res;
    }

private:
    bool has_res;
    json detect_json;
};
 
#endif // __AIIPC_H__