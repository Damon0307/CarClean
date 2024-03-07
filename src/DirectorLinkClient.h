#ifndef __DIRECTORLINKCLIENT_H__
#define __DIRECTORLINKCLIENT_H__

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include "json.hpp"
#include "Poco/Net/StreamSocket.h"
using json = nlohmann::json;

class DirectorLinkClient
{

public:
   DirectorLinkClient(const std::string cfg_json_file)
   {

  std::ifstream f(cfg_json_file);
  json data = json::parse(f);

  _ip = data["server_ip"];
  heartbeat_interval = data["heratbeat"];
  _port = data["server_port"];
  token_str = data["token"];
  xmbh_str = data["xmbh"]; 

   _ip = "192.168.169.1";
   _port = 9090;
   _socket = new Poco::Net::StreamSocket();
   }
   ~DirectorLinkClient()
   {
      if (_socket)
      {
         _socket->close();
      }
   }
   void ConnectToserver();

   void ReportCarPass(const json &data); 
   
   void ReportCarWashInfo(const json &data,bool is_detour = false);
   void ReportStatus();
   void RecvServerMessage(); 

private:
   std::string _ip;
   int _port;
   Poco::Net::StreamSocket *_socket;
   std::string messageType;
   std::string xmlData;
   int heartbeat_interval;
   std::string token_str;
   std::string xmbh_str;

   //util 函数 辅助转换过程
   int convertToDLAlarmType(const json &data);
   std::string convertLicensePlate(const std::string& licensePlate);
   int convertCarColor(const json&data);
   int convertCarType(const json&data);
//解析
   bool receiveAndParseMessage(Poco::Net::StreamSocket& socket, std::string& messageType, std::string& xmlData);
};

#endif // __DIRECTORLINKCLIENT_H__