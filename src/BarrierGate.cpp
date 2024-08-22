#include "BarrierGate.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "config.h"

unsigned int gpio_103_offset = 7; // IO1

unsigned int gpio_102_offset = 6; // IO2

int GPIO_InitLine(char *chipname, unsigned int offset, struct gpiod_line **line_ptr)
{
#if (ARM_FLAG == 1)
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip)
    {
        perror("Open chip failed\n");
        return -1;
    }

    *line_ptr = gpiod_chip_get_line(chip, offset); // IO1
    if (!(*line_ptr))
    {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return -1;
    }
#endif
    return 0;
}

//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)成功：0
//                2)失败：-1
// Parameters   : （形参列表）
//                const char *consumer :IO别名
//                int mode:输入输出模式（1为输出，0为输入）
//                struct gpiod_line **line_ptr：请求的GPIO线路结构体返回值
// -（函数功能说明）
//  （1）GPIO初始化
//-----------------------------------------------------------------------------
int GPIO_Set_Mode(struct gpiod_line **line_ptr, const char *consumer, int mode)
{
#if (ARM_FLAG == 1)
    struct gpiod_line *line = *line_ptr;
    int ret = -1;
    if (mode == 1)
    {
        ret = gpiod_line_request_output(line, consumer, 0); // IO1输出，IO1
        if (ret < 0)
        {
            perror("Request line as output failed\n");
            gpiod_line_release(line);
            return -1;
        }
    }
    else if (mode == 0)
    {
        ret = gpiod_line_request_input(line, consumer); // IO1输入，IO1
        if (ret < 0)
        {
            perror("Request line as input failed\n");
            gpiod_line_release(line);
            return -1;
        }
    }
    else
    {
        perror("Pattern error\n");
        return -1;
    }
#endif
    return 0;
}

//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)val：读取的数值（1：高电平，0：低电平）
// Parameters   : （形参列表）
//                struct gpiod_line **line_ptr：请求的GPIO线路结构体返回值
// -（函数功能说明）
//  （1）读取IO输入电平值
//-----------------------------------------------------------------------------
int GPIO_Read(struct gpiod_line **line_ptr)
{
#if (ARM_FLAG == 1)
    struct gpiod_line *line = *line_ptr;
    int val;
    val = gpiod_line_get_value(line);
    return val;
#endif
}

//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)成功：0
//                2)失败：-1
// Parameters   : （形参列表）
//                struct gpiod_line **line_ptr：请求的GPIO线路结构体返回值
//                int val:GPIO输出电平值
// -（函数功能说明）
//  （1）设置IO输出电平值
//-----------------------------------------------------------------------------
int GPIO_Write(struct gpiod_line **line_ptr, int val)
{
#if (ARM_FLAG == 1)
    struct gpiod_line *line = *line_ptr;
    int ret = gpiod_line_set_value(line, val);
    if (ret < 0)
    {
        perror("Set line value failed\n");
        gpiod_line_release(line);
        return -1;
    }
#endif
    return 0;
}

// 继电器部分开始

unsigned int gpio_71 = 7; // 继电器，DO
struct gpiod_line *line_71;

//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)成功：0
//                2)失败：-1
// Parameters   : （形参列表）
//                无
// -（函数功能说明）
//  （1）GPIO初始化
//-----------------------------------------------------------------------------
int relay_init()
{
#if(ARM_FLAG==1)    
    char *chipname = "gpiochip2";
    struct gpiod_chip *chip;

    int ret;
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip)
    {
        perror("Open chip failed\n");
        return -1;
    }

    line_71 = gpiod_chip_get_line(chip, gpio_71); // 继电器，DO
    if (!line_71)
    {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return -1;
    }

    ret = gpiod_line_request_output(line_71, "relay", 0); // 继电器，DO
    if (ret < 0)
    {
        perror("Request line as output failed\n");
        gpiod_line_release(line_71);
        return -1;
    }
#endif    
    return 0;
}
//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)成功：0
//                2)失败：-1
// Parameters   : （形参列表）
//                1)val：继电器开关值（0或1）
// -（函数功能说明）
//  （1）设置继电器开关
//-----------------------------------------------------------------------------
int write_relay(unsigned int val)
{
#if(ARM_FLAG==1)
    int ret;
    ret = gpiod_line_set_value(line_71, val);
    if (ret < 0)
    {
        perror("Set line output failed\n");
        gpiod_line_release(line_71);
        return -1;
    }
#endif 
    return 0;
}

// 继电器部分结束

BarrierGate::BarrierGate(/* args */)
{
    InitIO();
}

BarrierGate::~BarrierGate()
{
}

void BarrierGate::BarrierGateCtrl(bool ctrl_flag)
{
    // if(ctrl_flag)
    // {
    //      GPIO_Write(&lineIO1, 1);
    // }
    // else
    // {
    //     GPIO_Write(&lineIO1, 0);
    // }

    if (ctrl_flag)
    {
        write_relay(1);
    }
    else
    {
        write_relay(0);
    }
}
void BarrierGate::InitIO()
{
#if(ARM_FLAG==1)
    //    GPIO_InitLine("gpiochip3" ,gpio_103_offset,&lineIO1);

    //    GPIO_Set_Mode(&lineIO1,"IO1",1);

    //    GPIO_Write(&lineIO1, 0);

    // 采用继电器方式
    relay_init();
#endif    
}