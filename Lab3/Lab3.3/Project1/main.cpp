#define _CRT_SECURE_NO_WARNINGS
#include <pcap.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>

// ��̫��֡ͷ�ṹ
struct ethernet_header {
    u_char dest[6];  // Ŀ�� MAC ��ַ
    u_char src[6];   // Դ MAC ��ַ
    u_short type;    // ��̫������
};

// IP ͷ�ṹ
struct ip_header {
    u_char ver_ihl;        // �汾 (4 bits) + IP ͷ���� (4 bits)
    u_char tos;            // ��������
    u_short tlen;          // �ܳ���
    u_short identification; // ��ʶ
    u_short flags_fo;      // ��־ (3 bits) + Ƭƫ���� (13 bits)
    u_char ttl;            // ����ʱ��
    u_char proto;          // Э��
    u_short crc;           // ͷ��У���
    u_char saddr[4];       // Դ IP ��ַ
    u_char daddr[4];       // Ŀ�� IP ��ַ
};

// ��ȡ��ǰʱ���ַ���
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

// ת��MAC��ַ���ַ���
std::string macToString(const u_char* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

// ת��IP��ַ���ַ���
std::string ipToString(const u_char* ip) {
    char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return std::string(ipStr);
}

// �ص�����������������ݰ�
void packetHandler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data) {
    // ������̫��֡ͷ
    struct ethernet_header* eth = (struct ethernet_header*)pkt_data;

    // ֻ������̫������Ϊ IP (0x0800) �İ�
    if (ntohs(eth->type) != 0x0800) {
        return;
    }

    // ���� IP ͷ
    struct ip_header* ip = (struct ip_header*)(pkt_data + 14);  // ��̫��֡ͷΪ 14 �ֽ�

    // ��ȡ֡����
    int frame_length = header->len;

    // ��ȡʱ��
    std::string timeStr = getCurrentTime();

    // MAC ��ַ
    std::string srcMAC = macToString(eth->src);
    std::string destMAC = macToString(eth->dest);

    // IP ��ַ
    std::string srcIP = ipToString(ip->saddr);
    std::string destIP = ipToString(ip->daddr);

    // ������������̨
    std::cout << timeStr << "," << srcMAC << "," << srcIP << "," << destMAC << "," << destIP << "," << frame_length << std::endl;

    // �������� CSV �ļ�
    std::ofstream outfile;
    outfile.open("network_log.csv", std::ios::app);  // ׷��ģʽ
    outfile << timeStr << "," << srcMAC << "," << srcIP << "," << destMAC << "," << destIP << "," << frame_length << std::endl;
    outfile.close();
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];  // ���󻺳���
    pcap_if_t* alldevs;             // �����豸�б�
    pcap_if_t* d;                   // ��ǰ�豸
    int i = 0;                      // �豸����

    // ��ȡ�����豸�б�
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error finding devices: " << errbuf << std::endl;
        return 1;
    }

    // �г����п����豸
    std::cout << "Available Devices:" << std::endl;
    for (d = alldevs; d != nullptr; d = d->next) {
        std::cout << ++i << ": " << d->name;
        if (d->description) {
            std::cout << " (" << d->description << ")";
        }
        std::cout << std::endl;
    }

    // ѡ��һ���豸���м���
    int devNum;
    std::cout << "Enter the device number to capture: ";
    std::cin >> devNum;

    // ѡ���豸
    for (d = alldevs, i = 0; i < devNum - 1; d = d->next, i++);

    // ���豸���в���
    pcap_t* handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
    if (handle == nullptr) {
        std::cerr << "Error opening device: " << errbuf << std::endl;
        pcap_freealldevs(alldevs);
        return 1;
    }

    std::cout << "Capturing on device: " << d->name << std::endl;

    // �������ݰ�������
    pcap_loop(handle, 0, packetHandler, nullptr);

    // �ͷ��豸�б�
    pcap_freealldevs(alldevs);

    // �رղ�����
    pcap_close(handle);

    return 0;
}
