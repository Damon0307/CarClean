#ifndef __AIIPC_MANAGER_H__
#define __AIIPC_MANAGER_H__

/**
 * AIIPCManager - 管理所有 AI IPC 摄像头实例
 *
 * 当前启用: LEFT_WHEEL(左轮), RIGHT_WHEEL(右轮), TAIL(车尾)
 * 暂时屏蔽: SIDE_LEFT(左侧车身), SIDE_RIGHT(右侧车身)
 */

#include "AIIPC.h"
#include "httplib.h"

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

using namespace httplib;

class AIIPCManager
{
public:
    enum class Position {
        LEFT_WHEEL,
        RIGHT_WHEEL,
        // SIDE_LEFT,    // 暂时屏蔽
        // SIDE_RIGHT,   // 暂时屏蔽
        TAIL
    };

    AIIPCManager()
    {
        l_ai_ipc.SetAIIPCType(AIIPC::TYPE_WHEEL);
        r_ai_ipc.SetAIIPCType(AIIPC::TYPE_WHEEL);
        // side_l_ai_ipc.SetAIIPCType(AIIPC::TYPE_SIDE_BODY);    // 暂时屏蔽
        // side_r_ai_ipc.SetAIIPCType(AIIPC::TYPE_SIDE_BODY);    // 暂时屏蔽
        tail_ai_ipc.SetAIIPCType(AIIPC::TYPE_TAIL);
    }

    // --- 统一的数据处理入口 ---
    void DealAIIPCData(Position pos, const json &p_json,
                       bool point_b_is_working, time_t point_b_leave_time,
                       bool ipc_has_trigger, int ai_deal_delay_time)
    {
        if (!p_json.contains("label"))
            return;

        AIIPC *target = GetAIIPC(pos);

        if (pos == Position::TAIL)
        {
            // 车尾摄像头：基于 ipc 触发判断
            if (ipc_has_trigger)
            {
                target->DealAIIPCData(p_json);
                g_console_logger->debug("Deal Tail AIIPC clean res {}  ", p_json["label"].dump().c_str());
                g_file_logger->debug("Deal Tail AIIPC clean res {} ", p_json["label"].dump().c_str());
            }
            else
            {
                g_console_logger->debug("Rejected handle Tail AIIPC cause ipc has no trigger");
                g_file_logger->debug("Rejected handle Tail AIIPC cause ipc has no trigger");
            }
        }
        else
        {
            // 车轮摄像头：基于 point_b 工作状态判断
            if (point_b_is_working)
            {
                target->DealAIIPCData(p_json);
                LogDeal(pos, p_json, false);
            }
            else
            {
                time_t cur_time;
                time(&cur_time);
                if (difftime(cur_time, point_b_leave_time) <= ai_deal_delay_time)
                {
                    target->DealAIIPCData(p_json);
                    LogDeal(pos, p_json, true);
                }
                else
                {
                    g_console_logger->debug("Rejected handle {} AIIPC cause no point b working", PositionName(pos));
                    g_file_logger->debug("Rejected handle {} AIIPC cause no point b working", PositionName(pos));
                }
            }
        }
    }

    // --- 重置所有摄像机状态 ---
    void ResetAll()
    {
        l_ai_ipc.ResetStatus();
        r_ai_ipc.ResetStatus();
        // side_l_ai_ipc.ResetStatus();    // 暂时屏蔽
        // side_r_ai_ipc.ResetStatus();    // 暂时屏蔽
        tail_ai_ipc.ResetStatus();
    }

    // --- 结果获取 ---
    json GetLeftWheelDetectRes()   { return l_ai_ipc.GetDetectRes(); }
    json GetRightWheelDetectRes()  { return r_ai_ipc.GetDetectRes(); }
    json GetTailDetectRes()        { return tail_ai_ipc.GetDetectRes(); }
    // json GetSideLeftDetectRes()    { return side_l_ai_ipc.GetDetectRes(); }    // 暂时屏蔽
    // json GetSideRightDetectRes()   { return side_r_ai_ipc.GetDetectRes(); }    // 暂时屏蔽

    // --- 状态查询 ---
    bool IsLeftWheelReady()    { return l_ai_ipc.GetResult(); }
    bool IsRightWheelReady()   { return r_ai_ipc.GetResult(); }
    bool IsTailReady()         { return tail_ai_ipc.GetResult(); }
    // bool IsSideLeftReady()     { return side_l_ai_ipc.GetResult(); }    // 暂时屏蔽
    // bool IsSideRightReady()    { return side_r_ai_ipc.GetResult(); }    // 暂时屏蔽

    bool BothWheelsReady()
    {
        return l_ai_ipc.GetResult() && r_ai_ipc.GetResult();
    }

private:
    AIIPC *GetAIIPC(Position pos)
    {
        switch (pos)
        {
        case Position::LEFT_WHEEL:   return &l_ai_ipc;
        case Position::RIGHT_WHEEL:  return &r_ai_ipc;
        case Position::TAIL:         return &tail_ai_ipc;
        // case Position::SIDE_LEFT:    return &side_l_ai_ipc;
        // case Position::SIDE_RIGHT:   return &side_r_ai_ipc;
        default:                     return &l_ai_ipc;
        }
    }

    static const char *PositionName(Position pos)
    {
        switch (pos)
        {
        case Position::LEFT_WHEEL:   return "Left Wheel";
        case Position::RIGHT_WHEEL:  return "Right Wheel";
        case Position::TAIL:         return "Tail";
        // case Position::SIDE_LEFT:    return "Side Left";
        // case Position::SIDE_RIGHT:   return "Side Right";
        default:                     return "Unknown";
        }
    }

    void LogDeal(Position pos, const json &p_json, bool in_delay)
    {
        const char *name = PositionName(pos);
        if (in_delay)
        {
            g_console_logger->debug("Deal_{}_AIIPCData clean res in delay time {} ", name, p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_{}_AIIPCData clean res in delay time {} ", name, p_json["label"].dump().c_str());
        }
        else
        {
            g_console_logger->debug("Deal_{}_AIIPCData clean res {}  ", name, p_json["label"].dump().c_str());
            g_file_logger->debug("Deal_{}_AIIPCData clean res {} ", name, p_json["label"].dump().c_str());
        }
    }

private:
    AIIPC l_ai_ipc;
    AIIPC r_ai_ipc;
    // AIIPC side_l_ai_ipc;    // 暂时屏蔽
    // AIIPC side_r_ai_ipc;    // 暂时屏蔽
    AIIPC tail_ai_ipc;
};

#endif // __AIIPC_MANAGER_H__
