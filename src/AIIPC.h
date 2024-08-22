#ifndef __AIIPC_H__
#define __AIIPC_H__

/**
 * AI 摄像头模块抽象
 * 现在改成纯AI方案， 从冲洗摄像头被触发以后，拿取固定时长内的AI摄像头数据进行处理
 * 如果当中存在脏则判断为不干净，如果没有脏，则认为干净的标志
 * 保存数据的上限为 50个
 */
 
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include "json.hpp"
#include "time.h"
#include "spdlog/spdlog.h"  //导入日志

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

using json = nlohmann::json;

struct AIIPC_Data
{
    time_t timestamp; // 记录时间戳
    json res_json;    // AI IPC 数据    
};

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
        cur_dirty_img="";

        std::lock_guard<std::mutex> lock(mutex_aiipc_data); 
        //使用swap 进行清空容器
        std::deque<AIIPC_Data>().swap(aiipc_data_list);

    }
    void DealAIIPCData(const json &pjson);
    json GetDetectRes(int time_interval=10);

    bool GetResult()
    {
        return has_res;
    }

private:
    std::mutex mutex_aiipc_data;
    bool has_res;
    json detect_json;
    std::deque<AIIPC_Data> aiipc_data_list;
    std::string cur_dirty_img;
};

#endif // __AIIPC_H__