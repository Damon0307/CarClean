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
#include <net/route.h>
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
#include "WashReport.h"

// NTP时间戳是从1900年1月1日开始的秒数
#define NTP_TIMESTAMP_DELTA 2208988800ull
#define NTP_SERVER "36.156.67.46"
#define NTP_PORT 123

struct ntp_packet
{
  uint8_t li_vn_mode;
  uint8_t stratum;
  uint8_t poll;
  uint8_t precision;
  uint32_t root_delay;
  uint32_t root_dispersion;
  uint32_t ref_id;
  uint32_t ref_t_sec;
  uint32_t ref_t_frac;
  uint32_t orig_t_sec;
  uint32_t orig_t_frac;
  uint32_t rx_t_sec;
  uint32_t rx_t_frac;
  uint32_t tx_t_sec;
  uint32_t tx_t_frac;
};

using json = nlohmann::json;
using namespace httplib;
using namespace std;

extern std::shared_ptr<spdlog::logger> g_console_logger;
extern std::shared_ptr<spdlog::logger> g_file_logger;

#ifdef WASH_TEST_MODE
#include <deque>
#endif

bool updateConfigFile(const string &file_path, const string &new_content)
{
  ofstream file(file_path);
  if (!file.is_open())
  {
    return false;
  }
  file << new_content;
  file.close();

  system("sync");
  return true;
}

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
        g_console_logger->info("try to config ip to {}", this->local_server);
        g_file_logger->info("try to config ip to {}", this->local_server);
      }
      this_thread::sleep_for(chrono::seconds(30));
    } });
}

// === IPC Handler Methods（全部委托给GenericHandler） ===

void NetFoundation::WashIPCDataHandler(const Request &req, Response &res)
{
  GenericHandler(WASH_IPC, req, res);
}

void NetFoundation::DetourIPCDataHandler(const Request &req, Response &res)
{
  GenericHandler(DETOUR_IPC, req, res);
}

void NetFoundation::CarInIPCDataHandler(const Request &req, Response &res)
{
  GenericHandler(CAR_IN_IPC, req, res);
}

void NetFoundation::LeftWheelAIIPCHandler(const Request &req, Response &res)
{
  GenericHandler(LEFT_WHEEL_AI, req, res);
}

void NetFoundation::RightWheelAIIPCHandler(const Request &req, Response &res)
{
  GenericHandler(RIGHT_WHEEL_AI, req, res);
}

void NetFoundation::TailAIIPCHandler(const Request& req, Response& res)
{
  GenericHandler(TAIL_AI, req, res);
}

void NetFoundation::RoofAIIPCHandler(const Request& req, Response& res)
{
  GenericHandler(ROOF_AI, req, res);
}

void NetFoundation::LeftSideAIIPCHandler(const Request& req, Response& res)
{
  GenericHandler(LEFT_SIDE_AI, req, res);
}

void NetFoundation::RightSideAIIPCHandler(const Request& req, Response& res)
{
  GenericHandler(RIGHT_SIDE_AI, req, res);
}

#ifdef WASH_TEST_MODE
void NetFoundation::SetupTestMode(WashReport *wash_report)
{
  m_wash_report = wash_report;
  RegisterTestRoutes(*m_wash_report);
}

void NetFoundation::RegisterTestRoutes(WashReport &wash_report)
{
  // 串口协议帧注入：Body {"frame": [0x55, power_type, point_b_status, 0, 0, water_pump_status, crc_h, crc_l, 0xAA]}
  mServer.Post("/test/serial", [&wash_report](const Request &req, Response &res) {
    try
    {
      json body = json::parse(req.body);
      if (!body.contains("frame") || !body["frame"].is_array())
      {
        res.status = 400;
        res.set_content("Missing or invalid 'frame' array", "text/plain");
        return;
      }

      std::deque<char> queue;
      for (const auto &item : body["frame"])
      {
        queue.push_back(static_cast<char>(item.get<int>()) & 0xFF);
      }

      wash_report.InjectSerialFrame(queue);
      res.set_content("ok", "text/plain");
    }
    catch (const std::exception &e)
    {
      res.status = 500;
      res.set_content(std::string("Error: ") + e.what(), "text/plain");
    }
  });

  // 查询内部传感器状态
  mServer.Post("/test/status", [&wash_report](const Request &req, Response &res) {
    res.set_content(wash_report.GetSensorStatusJson().dump(), "application/json");
  });
}
#endif

bool NetFoundation::PostDataToServer(json p_json)
{
  httplib::Client cli(remote_server, remote_port);
  httplib::Headers headers = {
      {"Content-Type", "application/json"}};

  auto res = cli.Post("/chechong/upload", headers, p_json.dump(), "application/json");

  if (res && res->status == 200)
  {
    std::cout << "Got res from remote server" << std::endl;
    std::cout << res->body << std::endl;
    return true;
  }
  else
  {
    std::cout << "Error sending data\n";
    return false;
  }

  return false;
}

void NetFoundation::StartServer()
{
  // 注册 IPC 车牌识别 POST处理函数
  mServer.Post("/wash_report", [&](const Request &req, Response &res)
               { WashIPCDataHandler(req, res); });

  mServer.Post("/detour_report", [&](const Request &req, Response &res)
               { DetourIPCDataHandler(req, res); });

  mServer.Post("/aiipc/left", [&](const Request &req, Response &res)
               { LeftWheelAIIPCHandler(req, res); });
  mServer.Post("/aiipc/right", [&](const Request &req, Response &res)
               { RightWheelAIIPCHandler(req, res); });

  mServer.Post("/car_in", [&](const Request &req, Response &res)
               { CarInIPCDataHandler(req, res); });

  mServer.Post("/aiipc/tail", [&](const Request &req, Response &res)
               { RoofAIIPCHandler(req, res); });
  /*
  // === 暂时屏蔽: 车身两侧 ===
  mServer.Post("/aiipc/side_left", [&](const Request &req, Response &res)
               { LeftSideAIIPCHandler(req, res); });
  mServer.Post("/aiipc/side_right", [&](const Request &req, Response &res)
               { RightSideAIIPCHandler(req, res); });
  */
  // 保留旧路由兼容 (roof_and_tail → 现在只处理tail)
  mServer.Post("/aiipc/roof_and_tail", [&](const Request &req, Response &res)
               { RoofAIIPCHandler(req, res); });



  // 修改配置文件
  mServer.Post("/update_def", [](const Request &req, Response &res)
               {
      string new_content = req.body;
      string file_path ="/default_info.json";
      if (updateConfigFile(file_path, new_content)) {
          res.set_content("Configuration file updated successfully", "text/plain");
      } else {
          res.status = 500;
          res.set_content("Failed to update the file", "text/plain");
      } });

  // 获取默认配置文件内容
  mServer.Post("/get_def", [](const Request &req, Response &res)
               {
                 string file_path = "/default_info.json";
                 std::ifstream file(file_path);
                 if (!file.is_open())
                 {
                   res.status = 500;
                   res.set_content("Failed to open the file", "text/plain");
                   return;
                 }
                 std::stringstream buffer;
                 buffer << file.rdbuf();
                 res.set_content(buffer.str(), "text/plain");
               });

  // 客户端测试连接
  mServer.Post("/test", [](const Request &req, Response &res)
               {
          res.set_content("ok", "text/plain"); });

  // 重启
  mServer.Post("/reboot", [](const Request &req, Response &res)
               {
                 res.set_content("ok", "text/plain");
                 system("reboot"); });

  // 获取日志
  mServer.Post("/get_log", [](const httplib::Request &req, httplib::Response &res)
               {
                 printf("get_log\n");
                 std::ifstream log_file("/userdata/LogFile.log");
                 if (!log_file.is_open())
                 {
                   res.status = 500;
                   res.set_content("Failed to open log file", "text/plain");
                   return;
                 }

                 std::string buffer((std::istreambuf_iterator<char>(log_file)), std::istreambuf_iterator<char>());
                 std::vector<std::string> lines;
                 std::stringstream ss(buffer);
                 std::string line;
                 while (std::getline(ss, line))
                 {
                   lines.push_back(line);
                   if (lines.size() > 500)
                   {
                     lines.erase(lines.begin());
                   }
                 }
                 buffer.clear();
                 for (const auto &l : lines)
                 {
                   buffer += l + "\n";
                 }
                 res.set_content(buffer, "text/plain");
               });

  // OTA 更新
  mServer.Post("/update_process", [](const Request &req, Response &res)
               {
        printf("updateprocess\n");

        if (req.has_file("file")) {
            const auto& file = req.get_file_value("file");

            std::string filename = file.filename;
            if (filename.find("/") != std::string::npos || filename.find("..") != std::string::npos) {
                res.status = 400;
                res.set_content("Invalid file name", "text/plain");
                return;
            }

            std::string filepath = "/tmp/" + filename;
            std::cout << "Receiving file: " << filepath << std::endl;

            std::ofstream ofs(filepath, std::ios::binary);
            if (ofs.is_open()) {
                ofs.write(file.content.data(), file.content.size());
                ofs.close();

                res.status = 200;
                res.set_content("File uploaded successfully", "text/plain");

                std::string command2 = "chmod +x " + filepath;
                if (system(command2.c_str()) != 0) {
                    res.status = 500;
                    res.set_content("Failed to set execute permission", "text/plain");
                    return;
                }

                std::string command = "mv " + filepath + " /";
                if (system(command.c_str()) != 0) {
                    res.status = 500;
                    res.set_content("Failed to move file to /", "text/plain");
                    return;
                }

                system("sync");
                std::cout << "File saved to /tmp and execute permission set." << std::endl;
            } else {
                res.status = 500;
                res.set_content("Failed to save file", "text/plain");
            }
        } else {
            res.status = 400;
            res.set_content("No file data in request", "text/plain");
        } });

  // 清理崩溃日志
  mServer.Post("/clear_log", [](const Request &req, Response &res)
               {
          system("cd / && find   /mnt/sdcard/ -name \"core-*-CarClean\" -exec rm -f {} + ");
          res.set_content("ok", "text/plain"); });

  mServer.listen(local_server, local_port);
}

void NetFoundation::ConfigRV1106IP(const std::string &ip)
{
#if 1
  int ret = system("killall udhcpc");
  if (ret != 0)
  {
    g_console_logger->error("Failed to kill udhcpc process, system() returned {}", ret);
    g_file_logger->error("Failed to kill udhcpc process, system() returned {}", ret);
  }

  const char *eth_interface = "eth0";
  const char *static_ip = "192.168.1.200";
  const char *subnet_mask = "255.255.255.0";
  const char *gateway = "192.168.1.1";
  const char *dns_server = "8.8.8.8";

  system(("ifconfig " + std::string(eth_interface) + " " + std::string(static_ip) + " netmask " + std::string(subnet_mask)).c_str());
  system(("route add default gw " + std::string(gateway)).c_str());

  std::ofstream resolv_conf("/etc/resolv.conf");
  if (resolv_conf.is_open())
  {
    resolv_conf << "nameserver " << dns_server << std::endl;
    resolv_conf.close();
  }
#endif
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

  close(sock);
  return std::string(ip);
}

void NetFoundation::SyncTimeWithNTP()
{
  std::atomic<bool> running(true);

  std::thread([&]()
              {
        while (running) {
            int sockfd;
            struct sockaddr_in serv_addr;
            ntp_packet packet;
            socklen_t len = sizeof(serv_addr);
            struct timeval timeout;

            sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sockfd < 0) {
                std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
                continue;
            }

            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            memset(&packet, 0, sizeof(ntp_packet));
            packet.li_vn_mode = 0x1B;

            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(NTP_SERVER);
            serv_addr.sin_port = htons(NTP_PORT);

            if (sendto(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                std::cerr << "Error sending packet: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            if (recvfrom(sockfd, (char *)&packet, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr, &len) < 0) {
                if (errno == EWOULDBLOCK) {
                    std::cerr << "NTP request timed out." << std::endl;
                } else {
                    std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
                }
                close(sockfd);
                continue;
            }

            time_t tx_time = ntohl(packet.tx_t_sec) - NTP_TIMESTAMP_DELTA;
            struct timeval new_time;
            new_time.tv_sec = tx_time;
            new_time.tv_usec = 0;

            if (settimeofday(&new_time, NULL) < 0) {
                std::cerr << "Error setting system time: " << strerror(errno) << std::endl;
                close(sockfd);
                continue;
            }

            std::cout << "NTP System time set to: " << ctime(&new_time.tv_sec) << std::endl;
            close(sockfd);
            g_console_logger->info("NTP System time set to: {}", ctime(&new_time.tv_sec));
            g_file_logger->info("NTP System time set to: {}", ctime(&new_time.tv_sec));

            std::this_thread::sleep_for(std::chrono::minutes(5));
        } })
      .detach();
}
