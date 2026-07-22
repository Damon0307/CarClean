/**
 *
 * Copyright 2022 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: XPY <xupengyu@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#ifndef UART_H
#define UART_H


//串口相关的头文件    
#include<stdio.h>      /*标准输入输出定义*/    
#include<stdlib.h>     /*标准函数库定义*/    
#include<unistd.h>     /*Unix 标准函数定义*/    
#include<sys/types.h>     
#include<sys/stat.h>       
#include<fcntl.h>      /*文件控制定义*/    
#include<termios.h>    /*PPSIX 终端控制定义*/    
#include<errno.h>      /*错误号定义*/    
#include<string.h>    



#if defined(__cplusplus)
extern "C"{
#endif


#define FALSE  -1    
#define TRUE   0

extern int UART_Open(int fd,char*port);
extern void UART_Close(int fd) ; 
extern int UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
extern int UART_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity) ;
extern int UART_Recv(int fd, char *rcv_buf,int data_len);
extern int UART_Send(int fd, char *send_buf,int data_len);


#if defined(__cplusplus)
}
#endif

#endif