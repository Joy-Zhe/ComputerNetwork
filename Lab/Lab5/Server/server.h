//
// Created by Joy Zheng on 2023/10/14.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <process.h>
#include <string>
#include <vector>
#include <mutex>
#include <cdonts.h>
#include "parser.h"

using namespace std;
HANDLE g_Mutex;

class Server {
public:
    Server(int);
};



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
            send(clientList[i].sClient, msg, strlen(msg), 0);
            return;
        } else {
            continue;
        }
    }
    printf("No such client!\n");
}

bool HandleRequest(int type, const Msg& message, responseBody res, SOCKET sock) {
    struct responseBody *response = (struct responseBody *)malloc(sizeof(struct responseBody));
    response->type = type;
    strcpy(response->msg, message.data);
    cout << "type: " << type << endl;
    gethostname(response->name, sizeof(response->name));
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(response->time, sizeof(response->time), "%Y-%m-%d %H:%M:%S", timeInfo);

    char *responseBuffer = (char *)malloc(1024 * sizeof(char));
    struct responseBody *recvMsgClient = (struct responseBody *)malloc(sizeof(struct responseBody));
    if (response->type == 6) {
        DecodeInfo(response->msg, recvMsgClient);
        // send message to other clients
//        BroadcastMsg(recvMsgClient->ip, recvMsgClient->port, recvMsgClient->msg);
        return true;
    }
    EncodeResponse(response->type, response, responseBuffer);
//    send(conSock, responseBuffer, strlen(responseBuffer), 0);
//        send(conSock, "t", strlen("t"), 0);
    printf("Sent message: %s\n", responseBuffer);
//    std::cout << std::flush;
    memset((void *) &message, 0, sizeof(message));
    if (response->type == 2)
        return false;
    return false;
}

DWORD WINAPI RequestThread(LPVOID args) {
    SOCKET socket = *(SOCKET*)args;
    bool status = false;
    char * msg;
    char req[1024];
//    req.type = -1;
    req[0] = 't';
    int len;
    char *tmp;
    Msg message{};
    responseBody res{};
    cout << "print in thread" << endl;
    while ((len = recv(socket, req, sizeof(req), 0)) > 0) {
        cout << len << endl;
        if (len > 0)
        {
            req[len] = '\0';
            cout << req << endl;
            ParseRequest(req, &message);
            cout << req << endl;
            WaitForSingleObject(g_Mutex, INFINITE);  //等待互斥量触发
            // 调用HandleRequest对请求作出处理，对源客户端的回复储存在msg中
            status = HandleRequest(message.type, message, res, socket);
            ReleaseMutex(g_Mutex);  // 触发互斥量
            if (status == false) {
                return NULL;
            };

        }
        send(socket, "Hello", sizeof("Hello"), 0);
    }

}

void CreateThreadToHandleRequest(SOCKET socket) {
    DWORD threadID;
    HANDLE threadHandle;
    // 创建thread
    cout << "test" << endl;
    threadHandle = CreateThread(NULL, 0, RequestThread, &socket, 0, &threadID);
}

#endif //SERVER_SERVER_H
