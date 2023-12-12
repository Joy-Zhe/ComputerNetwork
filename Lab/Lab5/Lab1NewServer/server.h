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
#include <atomic>
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 4170 //listening port, my student ID

std::mutex mtx;
std::mutex cnt_mtx;
CRITICAL_SECTION cs;
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
//int cnt;
std::atomic<int> cnt;
DWORD WINAPI HandleClientRequest(LPVOID lpParam) {
    SOCKET conSock = (SOCKET)lpParam;

    Message message{};
    char buffer[1024];
    int bytes;
    int Flag = 1;

    while (Flag) {

        bytes = recv(conSock, buffer, sizeof(buffer), 0);
        if (bytes < 0) {
            std::cout << "Receive failed" << std::endl;
            break;
        }
        mtx.lock();
        Message rec{};
        ParseRequest(buffer, &rec);
//        char *typeStr = (char *)malloc(10 * sizeof(char));
//        char requestCpy[512]={};
//        strcpy(requestCpy, buffer);
//        typeStr = strtok(requestCpy, "|");
//        if (typeStr == nullptr) {
//            printf("Wrong request format!\n");
//            return -1;
//        }
//        rec.type = atoi(typeStr);
//        typeStr = strtok(nullptr, "|");
//        if (typeStr == nullptr) {
//            printf("Wrong request format!\n");
//            return -1;
//        }
//        strcpy(rec.data, typeStr);
//        printf("Parsed request: type = %d, data = %s\n", rec.type, rec.data);
//        free(typeStr);

        switch (rec.type) {
            case CANCEL: {
                for (size_t i = 0; i < clientList.size(); i++) {
                    if (clientList[i].sClient == conSock) {
                        clientList.erase(clientList.begin() + i);
                        break;
                    }
                }
                Flag = 0;
                closesocket(conSock);
                std::cout << "Connection closed" << std::endl;
                break;
            }
            case TIME: {
                time_t time1;
                time(&time1);
                Message mes;
                mes.type = TIME;
//                sprintf(mes.data, "%ld", time1);
                if (send(conSock, "test", sizeof("test"), 0) < 0) {
                    std::cout << "Error sending" << std::endl;
                }
                std::cout << "Time sent successfully" << std::endl;
                break;
            }
            case NAME: {
                Message mes;
                mes.type = NAME;
                gethostname(mes.data, sizeof(mes.data));
                if (send(conSock, (char*)&mes, sizeof(mes), 0) < 0) {
                    std::cout << "Error sending" << std::endl;
                }
                std::cout << "Name sent successfully" << std::endl;
                break;
            }
            case CLIENT: {
                Message mes;
                mes.type = CLIENT;
                strcpy(mes.data, "");
                for (size_t i = 0; i < clientList.size(); i++) {
                    int sock = clientList[i].sClient;
                    int port = clientList[i].port;
                    std::string addr = clientList[i].addr;
                    std::string ans = std::to_string(sock) + "," + std::to_string(port) + "," + addr + ";";
                    strcat(mes.data, ans.c_str());
                }
                if (send(conSock, (char*)&mes, sizeof(mes), 0) < 0) {
                    std::cout << "Error sending" << std::endl;
                }
                std::cout << "Client list sent successfully" << std::endl;
                break;
            }
            case SEND: {
                std::string content = std::string(rec.data);
                std::string addr = content.substr(0, content.find(","));
                content = content.substr(content.find(",") + 1);
                int port = atoi(content.substr(0, content.find(",")).c_str());
                content = content.substr(content.find(",") + 1);
                std::string ans = content;

                int flag = 0;
                for (size_t i = 0; i < clientList.size(); i++) {
                    if (clientList[i].addr == addr && clientList[i].port == port) {
                        Message mes;
                        mes.type = SEND;
                        strcpy(mes.data, content.c_str());
                        std::cout << clientList[i].sClient<< std::endl;
                        if (send(clientList[i].sClient, (char*)&mes, sizeof(mes), 0) < 0) {
                            std::cout << "Error sending" << std::endl;
                        }
                        std::cout << "Sent to IP: " + addr + " Port: " + std::to_string(port) << std::endl;
                        flag = 1;
                        break;
                    }
                }
                if (!flag) {
                    Message mes;
                    mes.type = SEND;
                    strcpy(mes.data, "no this port");
                    std::cout << "Destination not found" << std::endl;
                    send(conSock, (char*)&mes, sizeof(mes), 0);
                }
                break;
            }
            default:
                break;
        }
//        buffer[bytes] = '\0';
//        ParseRequest(buffer, &message);
//
////        std::cout << message.type << std::endl;
//        struct responseBody response;
//        response.type = message.type;
//        strcpy(response.msg, message.data);
//        gethostname(response.name, sizeof(response.name));
//        time_t currentTime;
//
//        struct tm *timeInfo;
//        time(&currentTime);
//        timeInfo = localtime(&currentTime);
//        strftime(response.time, sizeof(response.time), "%Y-%m-%d %H:%M:%S", timeInfo);
//
//        char responseBuffer[1024];
//        struct responseBody recvMsgClient;
//        if (response.type == 6) {
//            DecodeInfo(response.msg, &recvMsgClient);
//            // send message to other clients
//            BroadcastMsg(recvMsgClient.ip, recvMsgClient.port, recvMsgClient.msg);
//            continue;
//        }
//        EncodeResponse(response.type, &response, responseBuffer);
//        send(conSock, responseBuffer, strlen(responseBuffer), 0);
//        cnt_mtx.lock();
//        cnt++;
//        std::cout << "cnt" << cnt.load() << std::flush << std::endl;
//        cnt_mtx.unlock();
//        printf("Sent message: %s\n", responseBuffer);
        memset(&rec, 0, sizeof(rec));
        mtx.unlock();
//        if (response.type == 2)
//            break;
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
