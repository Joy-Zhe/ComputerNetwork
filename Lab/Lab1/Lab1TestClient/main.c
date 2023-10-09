#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1" // 服务器的 IP 地址
#define SERVER_PORT 4169       // 服务器的端口号

#define HEARTBEAT_INTERVAL 5000 // 心跳间隔，以毫秒为单位

void printMenu() {
    printf("Menu:\n");
    printf("1. Connect to server\n");
    printf("2. Disconnect from server\n");
    printf("3. Get server time\n");
    printf("4. Send heartbeat\n");
    printf("5. Quit\n");
    printf("Enter your choice: ");
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup() failed!\n");
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("socket() failed!\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    bool connected = false;

    while (1) {
        printMenu();

        int choice;
        char timeBuf[100];
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                if (!connected) {
                    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                        printf("connect() failed!\n");
                        Sleep(5000); // 重试连接的间隔，以毫秒为单位
                        continue;
                    }
                    connected = true;
                    printf("Connected to server.\n");
                } else {
                    printf("Already connected to server.\n");
                }
                break;

            case 2:
                if (connected) {
                    closesocket(clientSocket);
                    connected = false;
                    printf("Disconnected from server.\n");
                } else {
                    printf("Not connected to server.\n");
                }
                break;

            case 3:
                if (connected) {
                    const char *request = "|3|LocalTime|"; // 请求服务器获取时间
                    send(clientSocket, request, strlen(request), 0);

                    // 接收服务器的响应
                    char response[1024];
                    int bytesRead = recv(clientSocket, response, sizeof(response) - 1, 0);
                    if (bytesRead == SOCKET_ERROR) {
                        printf("recv() failed!\n");
                        closesocket(clientSocket);
                        connected = false;
                        Sleep(5000); // 重试连接的间隔，以毫秒为单位
                        continue;
                    }

                    response[bytesRead] = '\0'; // 确保字符串以 null 结尾
                    printf("Server response: %s\n", response);

                    printf("Server time: %s\n", timeBuf);
                } else {
                    printf("Not connected to server.\n");
                }
                break;

            case 4:
                if (connected) {
                    // 发送心跳消息的代码...
                    // 接收心跳响应并处理...
                    printf("Sent heartbeat to server.\n");
                } else {
                    printf("Not connected to server.\n");
                }
                break;

            case 5:
                if (connected) {
                    closesocket(clientSocket);
                }
                WSACleanup();
                printf("Quitting program.\n");
                return 0;

            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}
