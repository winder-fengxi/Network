#define _CRT_SECURE_NO_WARNINGS
#include <pcap.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <cstring>

// 以太网帧头结构
struct ethernet_header {
    u_char dest[6];  // 目标 MAC 地址
    u_char src[6];   // 源 MAC 地址
    u_short type;    // 以太网类型
};

// IP 头结构
struct ip_header {
    u_char ver_ihl;        // 版本 (4 bits) + IP 头长度 (4 bits)
    u_char tos;            // 服务类型
    u_short tlen;          // 总长度
    u_short identification; // 标识
    u_short flags_fo;      // 标志 (3 bits) + 片偏移量 (13 bits)
    u_char ttl;            // 生存时间
    u_char proto;          // 协议
    u_short crc;           // 头部校验和
    u_char saddr[4];       // 源 IP 地址
    u_char daddr[4];       // 目标 IP 地址
};

// 获取当前时间字符串
std::string getCurrentTime() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::stringstream ss;
    ss << (now->tm_year + 1900) << '-'
        << (now->tm_mon + 1) << '-'
        << now->tm_mday << ' '
        << now->tm_hour << ':'
        << now->tm_min << ':'
        << now->tm_sec;
    return ss.str();
}

// 转换 MAC 地址到字符串
std::string macToString(const u_char* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X-%02X-%02X-%02X-%02X-%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

// 转换 IP 地址到字符串
std::string ipToString(const u_char* ip) {
    char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d",
        ip[0], ip[1], ip[2], ip[3]);
    return std::string(ipStr);
}

// 处理捕获的数据包
void packetHandler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data) {
    // 解析以太网帧头
    struct ethernet_header* eth = (struct ethernet_header*)pkt_data;

    // 只处理以太网类型为 IP (0x0800) 的包
    if (ntohs(eth->type) != 0x0800) {
        return;
    }

    // 解析 IP 头
    struct ip_header* ip = (struct ip_header*)(pkt_data + 14);  // 以太网帧头为 14 字节

    // 获取 TCP 头，假设 TCP 头长度为 20 字节，跳过 IP 头和以太网帧头
    const u_char* tcp = pkt_data + 14 + (ip->ver_ihl & 0x0F) * 4;

    // 检查协议是否为 TCP，且目的端口为 21 (FTP)
    if (ip->proto == 6 && ntohs(*(u_short*)(tcp + 2)) == 21) {
        // 将 TCP 数据转换为字符串，假设最大数据包长度为 65536 字节
        const u_char* payload = tcp + 20; // TCP 头长度
        int payload_len = header->len - (14 + (ip->ver_ihl & 0x0F) * 4 + 20);

        // 查找 USER 和 PASS 命令
        std::string payloadStr((const char*)payload, payload_len);
        if (payloadStr.find("USER") != std::string::npos) {
            std::string username = payloadStr.substr(payloadStr.find("USER") + 5);
            username = username.substr(0, username.find('\r')); // 取到换行前的内容
            std::string timeStr = getCurrentTime();
            std::cout << timeStr << ", USER: " << username << std::endl;

            // 输出到 CSV 文件
            std::ofstream outfile("ftp_login_log.csv", std::ios::app);
            outfile << timeStr << ", " << username << ", " << "N/A" << ", " << "N/A" << std::endl;
            outfile.close();
        }
        else if (payloadStr.find("PASS") != std::string::npos) {
            std::string password = payloadStr.substr(payloadStr.find("PASS") + 5);
            password = password.substr(0, password.find('\r')); // 取到换行前的内容
            std::string timeStr = getCurrentTime();
            std::cout << timeStr << ", PASS: " << password << std::endl;

            // 输出到 CSV 文件
            std::ofstream outfile("ftp_login_log.csv", std::ios::app);
            outfile << timeStr << ", " << "N/A" << ", " << password << ", " << "N/A" << std::endl;
            outfile.close();
        }
    }
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];  // 错误缓冲区
    pcap_if_t* alldevs;             // 网络设备列表
    pcap_if_t* d;                   // 当前设备
    int i = 0;                      // 设备计数

    // 获取网络设备列表
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error finding devices: " << errbuf << std::endl;
        return 1;
    }

    // 列出所有可用设备
    std::cout << "Available Devices:" << std::endl;
    for (d = alldevs; d != nullptr; d = d->next) {
        std::cout << ++i << ": " << d->name;
        if (d->description) {
            std::cout << " (" << d->description << ")";
        }
        std::cout << std::endl;
    }

    // 选择一个设备进行监听
    int devNum;
    std::cout << "Enter the device number to capture: ";
    std::cin >> devNum;

    // 选择设备
    for (d = alldevs, i = 0; i < devNum - 1; d = d->next, i++);

    // 打开设备进行捕获
    pcap_t* handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
    if (handle == nullptr) {
        std::cerr << "Error opening device: " << errbuf << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }

    std::cout << "Capturing on device: " << d->name << std::endl;

    // 捕获数据包并处理
    pcap_loop(handle, 0, packetHandler, nullptr);

    // 释放设备列表
    pcap_freealldevs(alldevs);

    // 关闭捕获句柄
    pcap_close(handle);

    return 0;
}
