#ifndef LAB1NEWSERVER_SERVER_H
#define LAB1NEWSERVER_SERVER_H
#include <cstdio>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <process.h>
#include <windows.h>
#include "parser.h"
#include <mutex>
#include <vector>
#include <cstdlib>
#include <string>
#include <ctime>
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 4169 //listening port, my student ID

void UpdateMaxIndex() {
    if (connectedClients > maxIndex) {
        maxIndex = connectedClients;
    }
}

std::mutex mtx;

void InitWinSocket(){
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        printf("WSAStartup() failed!\n");
        exit(1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("Invalid WinSock version!\n");
        WSACleanup();
        exit(1);
    }
}

void CreateListeningSocket(SOCKET *ListenSocket, struct sockaddr_in *saServer) {
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE; // for wildcard IP address

    *ListenSocket = INVALID_SOCKET;
    *ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(*ListenSocket == INVALID_SOCKET)
    {
        WSACleanup();
        printf("socket() failed!\n");
        exit(1);
    }

    (*saServer).sin_family = AF_INET; // address family
    (*saServer).sin_port = htons(SERVER_PORT); // listening port
    (*saServer).sin_addr.S_un.S_addr = htonl(INADDR_ANY); // listening address
}

void BindSocket(SOCKET *ListenSocket, struct sockaddr_in *saServer) {
    int ret;
    ret = bind(*ListenSocket, (struct sockaddr *)saServer, sizeof(*saServer));
    if (ret == SOCKET_ERROR)
    {
        printf("bind() failed! code:%d\n", WSAGetLastError());
        closesocket(*ListenSocket);// close socket
        WSACleanup();
        return;
    }
}

void Listen(const SOCKET *sListen) {
    int ret;
    ret = listen(*sListen, 500);
    if (ret == SOCKET_ERROR)
    {
        printf("listen() failed! code:%d\n", WSAGetLastError());
        closesocket(*sListen);// close socket
        WSACleanup();
        return;
    }
}

void BroadcastMsg(const char *ip, const char *port, const char *msg) {
    for (int i = 0; i < clientList.size(); i++) {
        if (clientList[i].if_connected == 0) {
            continue;
        }
        char cur_ip[20];
        unsigned short cur_port = ntohs(clientList[i].saClient.sin_port);
        strcpy(cur_ip, inet_ntoa(clientList[i].saClient.sin_addr));
        if (strcmp(cur_ip, ip) == 0 && cur_port == atoi(port)) {
            printf("Send message to client: %s:%d\n", cur_ip, cur_port);
            send(clientList[i - 1].sClient, msg, strlen(msg), 0);
            return;
        } else {
            continue;
        }
    }
    printf("No such client!\n");
}

DWORD WINAPI HandleClientRequest(LPVOID lpParam) {
    SOCKET conSock = (SOCKET)lpParam;

    Message message;
    char buffer[256];
    int bytes;
    int Flag = 1;
    int bufferSize = 10000000; // 设置缓冲区大小（以字节为单位）
    setsockopt(conSock, SOL_SOCKET, SO_RCVBUF, (const char*)&bufferSize, sizeof(int));
    setsockopt(conSock, SOL_SOCKET, SO_SNDBUF, (const char*)&bufferSize, sizeof(int));

    while (Flag) {
        bytes = recv(conSock, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            std::cout << "Receive failed" << std::endl;
            break;
        }
        buffer[bytes] = '\0';
        ParseRequest(buffer, &message);
        mtx.lock();
        std::cout << message.type << std::endl;
        struct responseBody *response = (struct responseBody *)malloc(sizeof(struct responseBody));
        response->type = message.type;
        strcpy(response->msg, message.data);
//        strcpy(response->ip, inet_ntoa(saClient.sin_addr));
//        sprintf(response->port, "%d", ntohs(saClient.sin_port));
//            strcpy(response->name, "client");
        gethostname(response->name, sizeof(response->name));
        time_t currentTime;
        struct tm *timeInfo;
        time(&currentTime);
        timeInfo = localtime(&currentTime);
        strftime(response->time, sizeof(response->time), "%Y-%m-%d %H:%M:%S", timeInfo);
        // for test output
//        printf("response->type: %d\n", response->type);
//        printf("response->msg: %s\n", response->msg);
//        printf("response->ip: %s\n", response->ip);
//        printf("response->port: %s\n", response->port);
//        printf("response->name: %s\n", response->name);
//        printf("response->time: %s\n", response->time);

        char *responseBuffer = (char *)malloc(1024 * sizeof(char));
        struct responseBody *recvMsgClient = (struct responseBody *)malloc(sizeof(struct responseBody));
        if (response->type == 6) {
            DecodeInfo(response->msg, recvMsgClient);
            // send message to other clients
            BroadcastMsg(recvMsgClient->ip, recvMsgClient->port, recvMsgClient->msg);
            continue;
        }
        EncodeResponse(response->type, response, responseBuffer);
        send(conSock, responseBuffer, strlen(responseBuffer), 0);
//        send(conSock, "t", strlen("t"), 0);
        printf("Sent message: %s\n", responseBuffer);
        std::cout << std::flush;
        memset(&message, 0, sizeof(message));
        mtx.unlock();
        if (response->type == 2)

            break;
    }

    closesocket(conSock);
    mtx.lock();
    for (size_t i = 0; i < clientList.size(); i++) {
        if (clientList[i].sClient == conSock) {
            clientList.erase(clientList.begin() + i);
            break;
        }
    }
    mtx.unlock();
    closesocket(conSock);
    std::cout << "Connection closed" << std::endl;
    return 0;
}

#endif
