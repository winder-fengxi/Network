#include <iostream>
#include <thread>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib") // 链接 WinSock 库

#define PORT 8080
#define HEARTBEAT_TIMEOUT 60 // 心跳超时时间（秒）

std::map<std::string, int> license_map; // 序列号 -> 当前连接数
std::set<std::string> valid_licenses = { "1234567890", "0987654321" }; // 示例有效序列号
std::mutex map_mutex;
std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> heartbeat_map;

// 初始化 WinSock
void initialize_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// 检查心跳状态
void monitor_heartbeats() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        auto now = std::chrono::system_clock::now();
        std::unique_lock<std::mutex> lock(map_mutex);
        for (auto it = heartbeat_map.begin(); it != heartbeat_map.end();) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
            if (duration > HEARTBEAT_TIMEOUT) {
                std::cout << "License " << it->first << " timed out. Removing." << std::endl;
                license_map[it->first]--;
                it = heartbeat_map.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

// 处理客户端连接
void handle_client(SOCKET client_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    recv(client_socket, buffer, sizeof(buffer), 0);

    std::string command(buffer);
    std::string response;

    std::unique_lock<std::mutex> lock(map_mutex);

    if (command.find("VERIFY:") == 0) {
        std::string license_key = command.substr(7);
        if (valid_licenses.count(license_key) == 0) {
            response = "INVALID_LICENSE";
        }
        else if (license_map[license_key] >= 10) { // 假设最大连接数为10
            response = "DENIED";
        }
        else {
            license_map[license_key]++;
            heartbeat_map[license_key] = std::chrono::system_clock::now();
            response = "AUTHORIZED";
        }
    }
    else if (command.find("HEARTBEAT:") == 0) {
        std::string license_key = command.substr(10);
        if (heartbeat_map.find(license_key) != heartbeat_map.end()) {
            heartbeat_map[license_key] = std::chrono::system_clock::now();
            response = "ALIVE";
        }
        else {
            response = "NOT_FOUND";
        }
    }

    lock.unlock();
    send(client_socket, response.c_str(), response.size(), 0);
    closesocket(client_socket);
}

int main() {
    initialize_winsock();

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "License server is running on port " << PORT << std::endl;

    std::thread(monitor_heartbeats).detach();

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed!" << std::endl;
            continue;
        }
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
