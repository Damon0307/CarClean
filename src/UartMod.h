#ifndef __UARTMOD_H__
#define __UARTMOD_H__

#include <functional>
#include <thread>
#include <deque>
#include "uart.h"

using observer_cb_t =std::function<void(char*,int)>;
 
class UartMod
{
private:
    /* data */
    int rs232_fd;
    
    std::deque<observer_cb_t> observer_deque;
 
    std::thread recv_thread;
public:
    UartMod(/* args */);
     ~UartMod();
    void InitComm(const char* uart_cfg_file_path);
    void StartComm();
 
//添加观察者，应该就一个模块需要接受这个信息
    void AddRecvCB(std::function<void(char*buf,int len)> p_recv)
    {
        observer_deque.push_back(p_recv);
    }
    void SendToRs232(const char*buf,int len);
 
};




#endif // __UARTMOD_H__