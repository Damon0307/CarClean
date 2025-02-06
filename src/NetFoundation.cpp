#include <iostream>
#include <fstream>
#include <string>
#include <functional>
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
#include <thread>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <ctime>
#include <sys/time.h>

#include "spdlog/spdlog.h"
#include "json.hpp"
#include "NetFoundation.h"

// NTP时间戳是从1900年1月1日开始的秒数
#define NTP_TIMESTAMP_DELTA 2208988800ull

// NTP服务器地址
#define NTP_SERVER "36.156.67.46"

// NTP端口
#define NTP_PORT 123

// NTP消息结构
struct ntp_packet
{
  uint8_t li_vn_mode;       // Leap indicator, version and mode
  uint8_t stratum;          // Stratum level
  uint8_t poll;             // Poll interval
  uint8_t precision;        // Precision
  uint32_t root_delay;      // Root delay
  uint32_t root_dispersion; // Root dispersion
  uint32_t ref_id;          // Reference ID
  uint32_t ref_t_sec;       // Reference time-stamp seconds
  uint32_t ref_t_frac;      // Reference time-stamp fraction
  uint32_t orig_t_sec;      // Originate time-stamp seconds
  uint32_t orig_t_frac;     // Originate time-stamp fraction
  uint32_t rx_t_sec;        // Received time-stamp seconds
  uint32_t rx_t_frac;       // Received time-stamp fraction
  uint32_t tx_t_sec;        // Transmit time-stamp seconds
  uint32_t tx_t_frac;       // Transmit time-stamp fraction
};

using json = nlohmann::json;
using namespace httplib;
using namespace std;

// extern logger obj
extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

NetFoundation::NetFoundation(/* args */)
{
}

NetFoundation::~NetFoundation()
{
  ip_check_thread.join();
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

  ip_check_thread = std::thread([this]()
                                {
    while (true)
    { 
      std::string cur_ip = GetPhyIP("eth0");  
      if(cur_ip!=this->local_server)
      {
        g_console_logger->info("IP change from {} to {}", this->local_server, cur_ip);
        g_file_logger->info("IP change from {} to {}", this->local_server, cur_ip);
        ConfigRV1106IP(this->local_server);
      } 
      this_thread::sleep_for(chrono::seconds(5));  
    } });
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
  return 0;
}

void NetFoundation::ConfigRV1106IP(const std::string &ip)
{
  //  const char *iface = "eth0"; // 网络接口名称
  //     const char *ip = "192.168.1.200"; // 静态IP地址
  //     const char *netmask = "255.255.255.0"; // 子网掩码
  //     const char *gw = "192.168.1.1"; // 默认网关
#if 0
  const char *iface_name = "eth0";
  const char *ip_address = ip.c_str();
 
  const char *netmask = "255.255.255.0";
  const char *gateway = "192.168.2.1"; // 默认网关
  const char* dns_server = "8.8.8.8"; //默认DNSserver

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    perror("socket");
    
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
    
  }

  // Set IP address
  if (ioctl(sock, SIOCSIFADDR, &ifr) < 0)
  {
    perror("SIOCSIFADDR");
    close(sock);
    
  }

  // Convert netmask from string to binary form
  if (inet_aton(netmask, &sin->sin_addr) == 0)
  {
    perror("inet_aton");
    close(sock);
    
  }

  // Set netmask
  if (ioctl(sock, SIOCSIFNETMASK, &ifr) < 0)
  {
    perror("SIOCSIFNETMASK");
    close(sock);
    
  }

  close(sock);

  // Add default gateway
  int route_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (route_sock < 0)
  {
    perror("socket");
    
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
    
  }

  sin = (struct sockaddr_in *)&rt.rt_dst;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = INADDR_ANY;

  if (ioctl(route_sock, SIOCADDRT, &rt) < 0)
  {
    perror("SIOCADDRT");
    close(route_sock);
    
  }

  close(route_sock);

  //设置DNSserver 
      // 设置 DNS
    std::ofstream resolv_conf("/etc/resolv.conf");
    if (resolv_conf.is_open()) {
        resolv_conf << "nameserver " << dns_server << std::endl;
        resolv_conf.close();
    } else {
        // 错误处理
    }
#endif

  const char *eth_interface = "eth0";
  const char *static_ip = "192.168.1.200";
  const char *subnet_mask = "255.255.255.0";
  const char *gateway = "192.168.1.1";
  const char *dns_server = "8.8.8.8";

  // 设置 IP 地址
  system(("ifconfig " + std::string(eth_interface) + " " + std::string(static_ip) + " netmask " + std::string(subnet_mask)).c_str());

  // 添加默认网关
  system(("route add default gw " + std::string(gateway)).c_str());

  // 设置 DNS
  std::ofstream resolv_conf("/etc/resolv.conf");
  if (resolv_conf.is_open())
  {
    resolv_conf << "nameserver " << dns_server << std::endl;
    resolv_conf.close();
  }
  else
  {
    // 错误处理
  }
}

std::string NetFoundation::GetPhyIP(const std::string &interface)
{
  int sock;
  struct ifreq ifr;

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
  {
    perror("ioctl failed");
    exit(EXIT_FAILURE);
  }

  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ip, INET_ADDRSTRLEN);

  // printf("IP Address of %s: %s\n", interface, ip);

  close(sock);
  return std::string(ip);
}

// ntp时间同步 
void NetFoundation::SyncTimeWithNTP()
{
    std::atomic<bool> running(true);
    
    std::thread([&]() {
        while (running) {
            int sockfd;
            struct sockaddr_in serv_addr;
            ntp_packet packet;
            socklen_t len = sizeof(serv_addr);
            struct timeval timeout;

            // 创建UDP套接字
            sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sockfd < 0) {
                std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
                continue;
            }

            // 设置接收超时
            timeout.tv_sec = 5; // 5秒超时
            timeout.tv_usec = 0;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            // 初始化NTP请求包
            memset(&packet, 0, sizeof(ntp_packet));
            packet.li_vn_mode = 0x1B; // LI = 0 (no warning), VN = 3 (IPv4 only), Mode = 3 (Client Mode)

            // 设置NTP服务器地址
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(NTP_SERVER);
            serv_addr.sin_port = htons(NTP_PORT);

            // 发送NTP请求
            if (sendto(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                std::cerr << "Error sending packet: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            // 接收NTP响应
            if (recvfrom(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, &len) < 0) {
                if (errno == EWOULDBLOCK) {
                    std::cerr << "NTP request timed out." << std::endl;
                } else {
                    std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
                }
                close(sockfd);
                continue;
            }

            // // 转换时间戳并打印
            time_t tx_time = ntohl(packet.tx_t_sec) - NTP_TIMESTAMP_DELTA;
            struct timeval new_time;
            new_time.tv_sec = tx_time;  
            new_time.tv_usec = 0; // NTP时间戳不包含亚秒精度



//   // 转换时间戳并打印
// time_t tx_time = ntohl(packet.tx_t_sec) - NTP_TIMESTAMP_DELTA;
// // 加上8小时（8小时 * 60分钟 * 60秒）
// tx_time += 8 * 60 * 60;

// // 不需要特别处理24小时进制，因为time_t会自动处理日期的进位
// struct timeval new_time;
// new_time.tv_sec = tx_time;  
// new_time.tv_usec = 0; // NTP时间戳不包含亚秒精度



            if (settimeofday(&new_time, NULL) < 0) {
                std::cerr << "Error setting system time: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            // 打印新的系统时间
            std::cout << "NTP System time set to: " << ctime(&new_time.tv_sec) << std::endl;
            close(sockfd);
            g_console_logger->info("NTP System time set to: {}", ctime(&new_time.tv_sec));  
            g_file_logger->info("NTP System time set to: {}", ctime(&new_time.tv_sec));

            // 每隔5分钟同步一次
            std::this_thread::sleep_for(std::chrono::minutes(5));
        }
    }).detach();
}
