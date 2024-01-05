#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include "UartMod.h"

using json = nlohmann::json;
using namespace std;

UartMod::UartMod(/* args */)
{
    observer_deque.clear();
    rs232_fd = -1;
}
UartMod::~UartMod()
{
    if (recv_thread.joinable() == true)
    {
        recv_thread.join();
    }
    UART_Close(rs232_fd);
    observer_deque.clear();
}

void UartMod::StartComm()
{
    int len = 0;
    char recv_buf[256] = {0};
 
    while (1) //循环读取数据    
    {   
       // len  = UART_Recv(rs232_fd, recv_buf,sizeof(recv_buf));   
         len = read(rs232_fd,recv_buf,sizeof(recv_buf));    
      
        if(len >= 0)    
        {    
            // for(int i=0;i<len;i++)
            // {
            //     printf("serial recv %x\n",recv_buf[i]);
            // }

            printf("Got \n");

            for(auto& iter : observer_deque)
            {
                observer_cb_t p_cb = iter;
                p_cb(recv_buf,len);
            }
              printf("here we go\n");
           // len = UART_Send(rs232_fd,rcv_buf,sizeof(rcv_buf)); 
             memset(recv_buf,0,sizeof(recv_buf));   
             len =0;
        }    
        else    
        {    
            printf(" serial recv data error\n");    
        } 
    } 

     
}

void UartMod::InitComm(const char *uart_cfg_file_path)
{

    std::ifstream f(uart_cfg_file_path);
    json data = json::parse(f);

    for (auto i : data)
    {
        std::cout << i << std::endl;
    }
    string port_str = data["port"];
    #if 0
    int speed = data["speed"];
    int flow_ctrl = data["flow_ctrl"];
    int databits = data["databits"];
    int stopbits = data["stopbits"];
    string parity_str = data["parity"];
    char c = parity_str.at(0);
    int parity = int(c);

    rs232_fd = open(port_str.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0);
    if (rs232_fd < 0)
    {
        printf("open error!\n");
        exit(-1);
    }
    else
    {
        cout << "Open " << port_str << "  success! " << endl;
    }

    if (UART_Set(rs232_fd, speed, flow_ctrl, databits, stopbits, parity) == 0)
    {
        printf(" UART_Set ERR\n");
    }
#endif

    rs232_fd = UART_Open(rs232_fd, (char*)port_str.c_str()); // 打开串口，返回文件描述符

    UART_Init(rs232_fd, 115200, 0, 8, 1, 'N');
    printf("Set Port Exactly!\n");
    sleep(1);
   // UART_Send(rs232_fd, "hello", 6);

    // extern int UART_Recv(int fd, char *rcv_buf,int data_len);
}

void UartMod::SendToRs232(const char *buf, int len)
{
    if (UART_Send(rs232_fd, (char *)buf, len) == 0)
    {
        printf(" UART_Send err\n");
    }
}