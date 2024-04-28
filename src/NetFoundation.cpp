#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include "json.hpp"
#include "NetFoundation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <net/route.h> // 包含 struct rtentry 和 RTF_GATEWAY 定义

using json = nlohmann::json;
using namespace httplib;
using namespace std;

NetFoundation::NetFoundation(/* args */)
{
}

NetFoundation::~NetFoundation()
{
}

void NetFoundation::InitNetCFG(const char *file_name)
{

  std::ifstream f(file_name);
  json data = json::parse(f);

  local_server = data["local_server"];
  remote_server = data["remote_server"];
  local_port = data["local_port"];
  remote_port = data["remote_port"];
  std::cout << "local_server : " << local_server << " local port :" << local_port << std::endl;
  std::cout << "remote_server : " << remote_server << " remote_port:" << remote_port << std::endl;
  ConfigRV1106IP(local_server);
}

// 之前考虑有光电模块触发，但是如果反向可能会拍不到车牌，所以决定直接配置IPC为推送模式，不需要comet轮询中加入业务处理
void NetFoundation::WashIPCDataHandler(const Request &req, Response &res)
{
  auto body = req.body;
  json req_data = json::parse(body);
  wash_hadler_func(req_data, res);
}

void NetFoundation::DetourIPCDataHandler(const Request &req, Response &res)
{
  auto body = req.body;
  json req_data = json::parse(body);
  detour_hadler_func(req_data, res);
}

void NetFoundation::CarInIPCDataHandler(const Request &req, Response &res)
{
  auto body = req.body;
  json req_data = json::parse(body);
  car_in_ipc_func(req_data, res);
}

void NetFoundation::PostDataToServer(json p_json)
{
  // 创建客户端
  httplib::Client cli(remote_server, remote_port);
  // 设置请求头indicating JSON body
  httplib::Headers headers = {
      {"Content-Type", "application/json"}};

  // 发送POST请求与JSON body
  auto res = cli.Post("/chechong/upload", headers, p_json.dump(), "application/json");

  //! NB
  // httplib::Client cli("https://hwlock.br-app.cn", 8080, "./cert.pem", "./key.pem");

  // 处理响应
  if (res && res->status == 200)
  {
    std::cout << "Got res from remote server" << std::endl;
    std::cout << res->body << std::endl;
  }
  else
  {
    std::cout << "Error sending data\n";
  }
}

void NetFoundation::StartServer()
{

  // 注册 IPC 车牌识别  POST处理函数
  mServer.Post("/wash_report", [&](const Request &req, Response &res)
               { WashIPCDataHandler(req, res); });

  mServer.Post("/detour_report", [&](const Request &req, Response &res)
               { DetourIPCDataHandler(req, res); });

  mServer.Post("/aiipc/left", [&](const Request &req, Response &res)
               { LeftSideAIIPCDataHandler(req, res); });
  mServer.Post("/aiipc/right", [&](const Request &req, Response &res)
               { RightSideAIIPCDataHandler(req, res); });
  // 增加一个车辆入场的处理
  mServer.Post("/car_in", [&](const Request &req, Response &res)
               { CarInIPCDataHandler(req, res); });

  // 启动服务器
  mServer.listen(local_server, local_port);
}

// 注册AIIPC 的webhook 目前看来有两个IPC API
void NetFoundation::RegisterWebHookForAIIPC()
{
}

void NetFoundation::LeftSideAIIPCDataHandler(const Request &req, Response &res)
{
  // 获取请求body  // 解析json
  auto body = req.body;
  json req_data = json::parse(body);
  wash_l_aiipc_func(req_data, res);
}
void NetFoundation::RightSideAIIPCDataHandler(const Request &req, Response &res)
{
  // 获取请求body  // 解析json
  auto body = req.body;
  json req_data = json::parse(body);
  wash_r_aiipc_func(req_data, res);
}

int set_static_ip(const char *iface_name, const char *ip_address, const char *netmask, const char *gateway)
{
}

void NetFoundation::ConfigRV1106IP(const std::string &ip)
{
  //  const char *iface = "eth0"; // 网络接口名称
  //     const char *ip = "192.168.1.200"; // 静态IP地址
  //     const char *netmask = "255.255.255.0"; // 子网掩码
  //     const char *gw = "192.168.1.1"; // 默认网关

  const char *iface_name = "eth0";
  const char *ip_address = ip.c_str();
  const char *netmask = "255.255.255.0";
  const char *gateway = "192.168.1.1"; // 默认网关

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    perror("socket");
    exit(-1);
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, iface_name, IFNAMSIZ - 1);

  struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
  sin->sin_family = AF_INET;

  // Convert IP address and netmask from string to binary form
  if (inet_aton(ip_address, &sin->sin_addr) == 0)
  {
    perror("inet_aton");
    close(sock);
    exit(-1);
  }

  // Set IP address
  if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
  {
    perror("SIOCSIFADDR");
    close(sock);
    exit(-1);
  }

  // Convert netmask from string to binary form
  if (inet_aton(netmask, &sin->sin_addr) == 0)
  {
    perror("inet_aton");
    close(sock);
    exit(-1);
  }

  // Set netmask
  if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0)
  {
    perror("SIOCSIFNETMASK");
    close(sock);
    exit(-1);
  }

  close(sock);

  // Add default gateway
  int route_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (route_sock < 0)
  {
    perror("socket");
    exit(-1);
  }

  struct rtentry rt;
  memset(&rt, 0, sizeof(rt));
  rt.rt_flags = RTF_GATEWAY;

  sin = (struct sockaddr_in *)&rt.rt_gateway;
  sin->sin_family = AF_INET;
  if (inet_aton(gateway, &sin->sin_addr) == 0)
  {
    perror("inet_aton");
    close(route_sock);
    exit(-1);
  }

  sin = (struct sockaddr_in *)&rt.rt_dst;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = INADDR_ANY;

  if (ioctl(route_sock, SIOCADDRT, &rt) < 0)
  {
    perror("SIOCADDRT");
    close(route_sock);
    exit(-1);
  }

  close(route_sock);
}