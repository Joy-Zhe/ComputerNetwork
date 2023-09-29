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

struct ClientInfo {
    SOCKET sClient;
    struct sockaddr_in saClient;
    int if_connected;
};
CRITICAL_SECTION cs; // 临界区


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

unsigned int __stdcall HandleClient(void *clientInfoPtr) {
    struct ClientInfo *clientInfo = (struct ClientInfo *)clientInfoPtr;
    SOCKET socket = clientInfo->sClient;
    struct sockaddr_in saClient = clientInfo->saClient;

    // Information
    const char *msg = "Hello, I'm server!";
    send(socket, msg, strlen(msg), 0);

    while (1) {
        char buffer[1024];
        int bytesRead = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesRead == SOCKET_ERROR) {
            printf("recv() failed! code:%d\n", WSAGetLastError());
            break; // connection failed
        } else if (bytesRead == 0) {
            printf("Client has closed the connection.\n");
            break; // connect
        } else {
            // 处理接收到的数据
            buffer[bytesRead] = '\0'; // 确保字符串以 null 结尾
            struct Message *message = (struct Message *)malloc(sizeof(struct Message));
            ParseRequest(buffer, message);
            printf("Received message: %d\n", message->type);

            // get all request information in responseBody
            struct responseBody *response = (struct responseBody *)malloc(sizeof(struct responseBody));
            /*
             * struct responseBody{
             *  int type;       // type
             *  char msg[512];  // message for type 6
             *  char ip[20];    // ip for type 1 and 2 and 4 and 5
             *  char port[10];
             *  char name[20];  // name for type 4
             *  char time[20];
             *  struct ClientList clientList[10];   // client list for type 5
             *  };
             */
            response->type = message->type;
            strcpy(response->msg, message->data);
            strcpy(response->ip, inet_ntoa(saClient.sin_addr));
            sprintf(response->port, "%d", ntohs(saClient.sin_port));
            strcpy(response->name, "client");
            time_t currentTime;
            struct tm *timeInfo;
            time(&currentTime);
            timeInfo = localtime(&currentTime);
            strftime(response->time, sizeof(response->time), "%Y-%m-%d %H:%M:%S", timeInfo);
            printf("response->type: %d\n", response->type);
            printf("response->msg: %s\n", response->msg);
            printf("response->ip: %s\n", response->ip);
            printf("response->port: %s\n", response->port);
            printf("response->name: %s\n", response->name);
            printf("response->time: %s\n", response->time);

//            if (strcmp(buffer, "GET_TIME") == 0) {
//                // 发送当前时间
//                time_t currentTime;
//                struct tm *timeInfo;
//                time(&currentTime);
//                timeInfo = localtime(&currentTime);
//                char timeStr[64];
//                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
//                send(socket, timeStr, strlen(timeStr), 0);
//                send(socket, (const char *) connectedClients, sizeof(connectedClients), 0);
//            } else {
//                // 处理其他请求类型
//                // 这里可以根据不同的请求类型添加相应的逻辑
//                // 例如，处理获取名字、获取客户端列表等其他请求
//                // 并根据请求类型发送相应的响应数据
//            }
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
    struct ClientInfo clientInfo[FD_SETSIZE];

    // initialize clientInfo
    for (int i = 0; i < FD_SETSIZE; i++) {
        clientInfo[i].if_connected = 0;
    }

    // main process
    while(1){
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(ListenSocket, &readSet);
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clientInfo[i].if_connected) {
                FD_SET(clientInfo[i].sClient, &readSet);
            }
        }
        int maxSocket = (int)ListenSocket;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clientInfo[i].if_connected) {
                if ((int)clientInfo[i].sClient > maxSocket) {
                    maxSocket = (int)clientInfo[i].sClient;
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
                    if (!clientInfo[i].if_connected) {
                        clientInfo[i].sClient = clientSocket;
                        clientInfo[i].saClient = saClient;
                        clientInfo[i].if_connected = 1;
                        connectedClients++; // 增加连接的客户端数量
                        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, &clientInfo[i], 0, NULL);
                        if (thread == NULL) {
                            printf("Create thread failed!\n");
                            exit(1);
                        }
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

        // 实时输出当前连接的客户端数量
        printf("Connected clients: %d\n", connectedClients);
    }
    DeleteCriticalSection(&cs);
    closesocket(ListenSocket);
    WSACleanup();
    exit(0);
}

#endif //LAB1_SERVER_H
