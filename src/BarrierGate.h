#ifndef __BARRIERGATE_H__
#define __BARRIERGATE_H__

/**
 * @file
 * @brief 用于控制闸机， 如果后期闸机私有协议比较多考虑提升为基类
 *
 * @author Your Name
 * @date 2023-02-24
 */


#include <iostream>
#include <string>
#include "gpiod.h"

class BarrierGate
{

public:
    BarrierGate(/* args */);
    ~BarrierGate();

    void BarrierGateCtrl(bool ctrl_flag);

private:
    /* data */

    struct gpiod_line *lineIO1;

    void InitIO();

};



#endif // __BARRIERGATE_H__