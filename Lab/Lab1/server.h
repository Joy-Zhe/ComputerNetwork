#ifndef LAB1_SERVER_H
#define LAB1_SERVER_H
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include "parser.h"
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 4169 //listening port, my student ID


CRITICAL_SECTION cs; // 临界区

void UpdateMaxIndex() {
    if (connectedClients > maxIndex) {
        maxIndex = connectedClients;
    }
}

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
    ret = listen(*sListen, 5);
    if (ret == SOCKET_ERROR)
    {
        printf("listen() failed! code:%d\n", WSAGetLastError());
        closesocket(*sListen);// close socket
        WSACleanup();
        return;
    }
}

// for Send Message to other clients
void BroadcastMsg(const char *ip, const char *port, const char *msg) {
    for (int i = 1; i <= maxIndex; i++) {
        if (clientList[i - 1].if_connected == 0) {
            continue;
        }
        char cur_ip[20];
        unsigned short cur_port = ntohs(clientList[i - 1].saClient.sin_port);
        strcpy(cur_ip, inet_ntoa(clientList[i - 1].saClient.sin_addr));
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

unsigned int __stdcall HandleClient(void *clientInfoPtr) {
    struct ClientInfo *clientInfo = (struct ClientInfo *)clientInfoPtr;
    SOCKET socket = clientInfo->sClient;
    struct sockaddr_in saClient = clientInfo->saClient;
//    strcpy(clientList[connectedClients].ip, inet_ntoa(saClient.sin_addr));
//    clientList[connectedClients].port = ntohs(saClient.sin_port);

    // Information
    const char *msg = "Hello, I'm server!";
//    send(socket, msg, strlen(msg), 0);

    while (1) {
        char buffer[1024];
        int bytesRead = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesRead == SOCKET_ERROR) {
            printf("recv() failed! code:%d\n", WSAGetLastError());
            break; // connection failed
        }
//        else if (bytesRead == 0) {
//            printf("Client has closed the connection.\n");
//            break; // connect
//        }
        else {
            // 处理接收到的数据
            buffer[bytesRead] = '\0'; // 确保字符串以 null 结尾
            struct Message *message = (struct Message *)malloc(sizeof(struct Message));
            ParseRequest(buffer, message);
            printf("Received message: %d\n", message->type);

            // get all request information in responseBody
            struct responseBody *response = (struct responseBody *)malloc(sizeof(struct responseBody));
            response->type = message->type;
            strcpy(response->msg, message->data);
            strcpy(response->ip, inet_ntoa(saClient.sin_addr));
            sprintf(response->port, "%d", ntohs(saClient.sin_port));
//            strcpy(response->name, "client");
            gethostname(response->name, sizeof(response->name));
            time_t currentTime;
            struct tm *timeInfo;
            time(&currentTime);
            timeInfo = localtime(&currentTime);
            strftime(response->time, sizeof(response->time), "%Y-%m-%d %H:%M:%S", timeInfo);
            // for test output
            /*printf("response->type: %d\n", response->type);
            printf("response->msg: %s\n", response->msg);
            printf("response->ip: %s\n", response->ip);
            printf("response->port: %s\n", response->port);
            printf("response->name: %s\n", response->name);
            printf("response->time: %s\n", response->time);*/

            char *responseBuffer = (char *)malloc(1024 * sizeof(char));
            struct responseBody *recvMsgClient = (struct responseBody *)malloc(sizeof(struct responseBody));
            if (response->type == 6) {
                DecodeInfo(response->msg, recvMsgClient);
                // send message to other clients
                BroadcastMsg(recvMsgClient->ip, recvMsgClient->port, recvMsgClient->msg);
            }
            EncodeResponse(response->type, response, responseBuffer);
            send(socket, responseBuffer, strlen(responseBuffer), 0);
            printf("Sent message: %s\n", responseBuffer);
            if (response->type == 2)
                break;
        }
    }

    // 关闭连接
    closesocket(socket);
    clientInfo->if_connected = 0;
    connectedClients--; // 减少连接的客户端数量
    printf("Connection closed for client: %s:%d\n",
           inet_ntoa(saClient.sin_addr), ntohs(saClient.sin_port));
    printf("Connected clients: %d\n", connectedClients);
    return 0;
}


void StartServer() {
    InitWinSocket();
    SOCKET ListenSocket;
    struct sockaddr_in saServer;
    CreateListeningSocket(&ListenSocket, &saServer);
    BindSocket(&ListenSocket, &saServer);
    Listen(&ListenSocket);
    printf("Waiting for client connecting!\n");
    printf("tips : Ctrl+c to quit!\n");
    InitializeCriticalSection(&cs); // 初始化临界区

    // initialize clientInfo
    for (int i = 0; i < FD_SETSIZE; i++) {
        clientList[i].if_connected = 0;
    }

    // main process
    while(1){
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(ListenSocket, &readSet);
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clientList[i].if_connected) {
                FD_SET(clientList[i].sClient, &readSet);
            }
        }
        int maxSocket = (int)ListenSocket;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clientList[i].if_connected) {
                if ((int)clientList[i].sClient > maxSocket) {
                    maxSocket = (int)clientList[i].sClient;
                }
            }
        }
        // select
        if (select(maxSocket + 1, &readSet, NULL, NULL, NULL) < 0) {
            printf("select() failed!\n");
            exit(1);
        }

        // check if there is a new connection
        if (FD_ISSET(ListenSocket, &readSet)) {
            struct sockaddr_in saClient;
            int length = sizeof(saClient);
            SOCKET clientSocket = accept(ListenSocket, (struct sockaddr *)&saClient, &length);
            if (clientSocket == INVALID_SOCKET) {
                printf("accept() failed!\n");
                exit(1);
            } else {
                int i;
                for (i = 0; i < FD_SETSIZE; i++) {
                    if (!clientList[i].if_connected) {
                        clientList[i].sClient = clientSocket;
                        clientList[i].saClient = saClient;
                        clientList[i].if_connected = 1;
                        connectedClients++; // number of clients
                        UpdateMaxIndex(); // update max index for indexing all clients
                        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, &clientList[i], 0, NULL);
                        if (thread == NULL) {
                            printf("Create thread failed!\n");
                            exit(1);
                        }
                        send(clientSocket, "Hello, I'm server!", strlen("Hello, I'm server!"), 0);
                        break;
                    }
                }
                if (i == FD_SETSIZE) {
                    printf("Too many clients!\n");
                    exit(1);
                }
                printf("Accepted client: %s:%d\n",
                       inet_ntoa(saClient.sin_addr), ntohs(saClient.sin_port));
            }
        }

        // output the number of connected clients
        printf("Connected clients: %d\n", connectedClients);
    }
    DeleteCriticalSection(&cs);
    closesocket(ListenSocket);
    WSACleanup();
    exit(0);
}

#endif //LAB1_SERVER_H
