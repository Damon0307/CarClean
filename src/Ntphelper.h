#ifndef NTP_TIME_SYNC_H
#define NTP_TIME_SYNC_H

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <atomic>

/**
 * NTP时间同步工具类
 * 功能：每2分钟从cn.ntp.org.cn同步一次时间，并设置到系统时间（东八区）
 */
class NTPTimeSync {
private:
    static constexpr const char* NTP_SERVER = "cn.ntp.org.cn";
    static constexpr int NTP_PORT = 123;
    static constexpr int TIMEOUT_SEC = 10;
    static constexpr int SYNC_INTERVAL_MIN = 2;
    static constexpr int TIMEZONE_OFFSET = 8; // 东八区
    
    // NTP数据包结构体
    struct NTPPacket {
        uint8_t li_vn_mode;      // LI(2bit) + VN(3bit) + Mode(3bit)
        uint8_t stratum;         // 层级
        uint8_t poll;            // 轮询间隔
        uint8_t precision;       // 精度
        uint32_t root_delay;     // 根延迟
        uint32_t root_dispersion; // 根离散
        uint32_t ref_id;         // 参考ID
        uint32_t ref_timestamp_sec;  // 参考时间戳秒
        uint32_t ref_timestamp_frac; // 参考时间戳小数
        uint32_t orig_timestamp_sec;  // 原始时间戳秒
        uint32_t orig_timestamp_frac; // 原始时间戳小数
        uint32_t recv_timestamp_sec;  // 接收时间戳秒
        uint32_t recv_timestamp_frac; // 接收时间戳小数
        uint32_t trans_timestamp_sec; // 传输时间戳秒
        uint32_t trans_timestamp_frac; // 传输时间戳小数
        
        NTPPacket() {
            memset(this, 0, sizeof(NTPPacket));
            li_vn_mode = 0x1B; // LI=0, VN=3, Mode=3 (client)
        }
    };
    
    std::atomic<bool> running_;
    std::thread sync_thread_;
    
    // 网络字节序转换
    uint32_t ntohl_custom(uint32_t netlong) {
        return ntohl(netlong);
    }
    
    uint32_t htonl_custom(uint32_t hostlong) {
        return htonl(hostlong);
    }
    
    // 获取NTP时间戳
    uint64_t getNTPTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto fraction = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);
        
        // NTP时间戳从1900年1月1日开始，Unix时间戳从1970年1月1日开始
        // 相差70年 = 2208988800秒
        uint64_t ntp_seconds = seconds.count() + 2208988800ULL;
        uint64_t ntp_fraction = (fraction.count() * 4294967296ULL) / 1000000ULL;
        
        return (ntp_seconds << 32) | ntp_fraction;
    }
    
    // 解析服务器地址
    bool resolveServer(const std::string& hostname, std::string& ip) {
        struct hostent* host_entry = gethostbyname(hostname.c_str());
        if (host_entry == nullptr) {
            std::cerr << "DNS解析失败: " << hostname << std::endl;
            return false;
        }
        
        ip = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
        return true;
    }
    
    // 从NTP服务器获取时间
    bool getNTPTime(time_t& server_time) {
        std::string server_ip;
        if (!resolveServer(NTP_SERVER, server_ip)) {
            return false;
        }
        
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "创建套接字失败" << std::endl;
            return false;
        }
        
        // 设置超时
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        // 服务器地址
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(NTP_PORT);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
        
        // 准备NTP数据包
        NTPPacket packet;
        uint64_t orig_timestamp = getNTPTimestamp();
        packet.orig_timestamp_sec = htonl_custom(orig_timestamp >> 32);
        packet.orig_timestamp_frac = htonl_custom(orig_timestamp & 0xFFFFFFFF);
        
        // 发送请求
        if (sendto(sockfd, &packet, sizeof(packet), 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "发送NTP请求失败" << std::endl;
            close(sockfd);
            return false;
        }
        
        // 接收响应
        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(sockfd, &packet, sizeof(packet), 0, 
                     (struct sockaddr*)&server_addr, &addr_len) < 0) {
            std::cerr << "接收NTP响应失败" << std::endl;
            close(sockfd);
            return false;
        }
        
        close(sockfd);
        
        // 解析时间戳
        uint32_t trans_sec = ntohl_custom(packet.trans_timestamp_sec);
        
        // 转换为Unix时间戳（减去70年的秒数）
        server_time = trans_sec - 2208988800ULL;
        
        // 添加东八区时区偏移（8小时 = 28800秒）
        server_time += (TIMEZONE_OFFSET * 3600);
        
        return true;
    }
    
    // 设置系统时间
    bool setSystemTime(time_t new_time) {
        struct timeval tv;
        tv.tv_sec = new_time;
        tv.tv_usec = 0;
        
        if (settimeofday(&tv, nullptr) != 0) {
            std::cerr << "设置系统时间失败: " << strerror(errno) << std::endl;
            return false;
        }
        
        return true;
    }
    
    // 同步时间
    void syncTime() {
        time_t server_time;
        if (getNTPTime(server_time)) {
            time_t current_time = time(nullptr);
            int time_diff = abs(server_time - current_time);
            
            // 如果时间差超过1秒，则更新系统时间
            if (time_diff > 1) {
                if (setSystemTime(server_time)) {
                    struct tm* timeinfo = localtime(&server_time);
                    char time_str[100];
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
                    
                    std::cout << "时间同步成功: " << time_str 
                              << " (调整了 " << time_diff << " 秒)" << std::endl;
                } else {
                    std::cerr << "时间同步失败：无法设置系统时间" << std::endl;
                }
            } else {
                std::cout << "时间已同步，无需调整" << std::endl;
            }
        } else {
            std::cerr << "获取NTP时间失败" << std::endl;
        }
    }
    
    // 同步线程函数
    void syncThreadFunc() {
        while (running_) {
            try {
                syncTime();
            } catch (const std::exception& e) {
                std::cerr << "时间同步异常: " << e.what() << std::endl;
            }
            
            // 等待2分钟
            for (int i = 0; i < SYNC_INTERVAL_MIN * 60 && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
public:
    NTPTimeSync() : running_(false) {}
    
    ~NTPTimeSync() {
        stop();
    }
    
    // 启动时间同步服务
    bool start() {
        if (running_) {
            std::cout << "时间同步服务已在运行" << std::endl;
            return true;
        }
        
        std::cout << "启动NTP时间同步服务..." << std::endl;
        std::cout << "NTP服务器: " << NTP_SERVER << std::endl;
        std::cout << "同步间隔: " << SYNC_INTERVAL_MIN << " 分钟" << std::endl;
        std::cout << "时区: 东八区 (UTC+8)" << std::endl;
        
        // 立即执行一次同步
        syncTime();
        
        running_ = true;
        sync_thread_ = std::thread(&NTPTimeSync::syncThreadFunc, this);
        
        std::cout << "NTP时间同步服务启动成功" << std::endl;
        return true;
    }
    
    // 停止时间同步服务
    void stop() {
        if (!running_) {
            return;
        }
        
        std::cout << "停止NTP时间同步服务..." << std::endl;
        running_ = false;
        
        if (sync_thread_.joinable()) {
            sync_thread_.join();
        }
        
        std::cout << "NTP时间同步服务已停止" << std::endl;
    }
    
    // 手动同步一次时间
    void syncOnce() {
        std::cout << "手动执行时间同步..." << std::endl;
        syncTime();
    }
    
    // 检查服务状态
    bool isRunning() const {
        return running_;
    }
};

#endif // NTP_TIME_SYNC_H
