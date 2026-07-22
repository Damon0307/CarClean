#ifndef __DIRECTORLINKCLIENT_H__
#define __DIRECTORLINKCLIENT_H__

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <functional>
#include "tinyxml2.h"

#include "json.hpp"

using json = nlohmann::json;

class DirectorLinkClient
{

public:
   DirectorLinkClient(const std::string cfg_json_file)
   {

//   std::ifstream f(cfg_json_file);
//   json data = json::parse(f);

//   _ip = data["server_ip"];
//   heartbeat_interval = data["heartbeat"];
//   _port = data["server_port"];
  // token_str = data["token"];
   token_str = "m8ac9ca5-11c1-7d272-2d52-29bfeafr6adp";
  // xmbh_str = data["xmbh"]; 
   xmbh_str =  "XMBH00000003";
 
   // ip = "192.168.169.1";
   // port = 9090;
    
   ConnectToserver();

   }
   ~DirectorLinkClient()
   {
     close(socket_fd);
   }
   void ConnectToserver();

   void ReportCarPass(const json &data,bool is_in); 
   
   void ReportCarWashInfo(const json &data,bool is_detour = false);
   void ReportStatus(const std::string& device_no,int status);
   
    
  //接收和解析
   void receiveAndParseMessage();
private:
   std::mutex mtx;
   int socket_fd; 

   std::string messageType;
   std::string xmlData;
   int heartbeat_interval;
   std::string token_str;
   std::string xmbh_str;
   
   int SendData(const std::string& data);
   //util 函数 辅助转换过程
   int convertToDLAlarmType(const json &data);
   std::string convertLicensePlate(const std::string& licensePlate);
   int convertCarColor(const json&data);
   int convertCarType(const json&data);
   int convertCarCleanResL(const json&data);
   int convertCarCleanResR(const json&data);

   // 函数将日期时间字符串转换为指定格式的字符串
std::string formatDateTime(const std::string& dateTime);

 // 函数将日期时间字符串转换为指定秒数后的日期时间字符串
std::string addSeconds(const std::string& dateTime, int secondsToAdd);
 

};

#endif // __DIRECTORLINKCLIENT_H__