#include <iostream>
#include <cstring>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib") // Á´½Ó WinSock ¿â

#define SERVER_IP "127.0.0.1"
#define PORT 8080

void initialize_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void send_heartbeat(const std::string& license_key) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed!" << std::endl;
            continue;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed!" << std::endl;
            closesocket(client_socket);
            continue;
        }

        std::string heartbeat_msg = "HEARTBEAT:" + license_key;
        send(client_socket, heartbeat_msg.c_str(), heartbeat_msg.size(), 0);

        char buffer[1024] = { 0 };
        recv(client_socket, buffer, sizeof(buffer), 0);
        std::cout << "Heartbeat response: " << buffer << std::endl;
        closesocket(client_socket);
    }
}

int main() {
    initialize_winsock();

    std::string license_key;
    std::cout << "Enter license key: ";
    std::cin >> license_key;

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    std::string verify_msg = "VERIFY:" + license_key;
    send(client_socket, verify_msg.c_str(), verify_msg.size(), 0);

    char buffer[1024] = { 0 };
    recv(client_socket, buffer, sizeof(buffer), 0);
    std::string response(buffer);
    std::cout << "Server response: " << response << std::endl;

    closesocket(client_socket);

    if (response == "AUTHORIZED") {
        std::thread(send_heartbeat, license_key).detach();
    }

    while (response == "AUTHORIZED") {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    WSACleanup();
    return 0;
}
