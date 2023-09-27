#ifndef LAB1_SERVER_H
#define LAB1_SERVER_H
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <time.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 6666 //listening port, my student ID
struct student
{
    char name[32];
    int age;
};

struct ClientInfo {
    SOCKET sCilent;
    struct sockaddr_in saClient;
    int if_connected;
};

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
//    SOCKET ListenSocket, sServer; //listening socket，connection socket
//    struct sockaddr_in saServer, saClient; // address info
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE; // for wildcard IP address

//    int iResult = getaddrinfo(NULL, "6666", &hints, &result);
//    if (iResult != 0) {
//        printf("getaddrinfo() failed!\n");
//        WSACleanup();
//        exit(1);
//    }

    *ListenSocket = INVALID_SOCKET;
//    *ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
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

void WaitForConnection(SOCKET *sListen, SOCKET *sServer, struct sockaddr_in *saClient) {
    int length = sizeof(*saClient);
    *sServer = accept(*sListen, (struct sockaddr *)saClient, &length);
//    if(*sServer == INVALID_SOCKET)
//    {
//        printf("accept() failed! code:%d\n", WSAGetLastError());
//        closesocket(*sListen);// close socket
//        WSACleanup();
//        return;
//    }

    while (*sServer == INVALID_SOCKET) {
        printf("accept() failed! code:%d\n", WSAGetLastError());
    }
    printf("Accepted client: %s:%d\n",
           inet_ntoa((*saClient).sin_addr), ntohs((*saClient).sin_port));
}

void Interaction(struct student stu, SOCKET *sServer, SOCKET *sListen, char **ptr) {
    int ret, nLeft = sizeof(stu);
    *ptr = (char *)&stu;
    while(nLeft >0)
    {
        //接收数据：
        ret = recv(*sServer, *ptr, nLeft, 0);
        if(ret == SOCKET_ERROR)
        {
            printf("recv() failed!\n");
            break;
        }

        if(ret == 0) //客户端已经关闭连接
        {
            printf("client has closed the connection!\n");
            break;
        }
        nLeft -= ret;
        *ptr += ret;
    }

    if(!nLeft) // all data received
        printf("name: %s\nage:%d\n", stu.name, stu.age);

    closesocket(*sListen);// close socket
    closesocket(*sServer);
    WSACleanup();
}

unsigned int __stdcall HandleClient(void *clientInfoPtr) {
    struct ClientInfo *clientInfo = (struct ClientInfo *)clientInfoPtr;
    SOCKET socket = clientInfo->sCilent;
    struct sockaddr_in saClient = clientInfo->saClient;

    // information
    const char *msg = "Hello, I'm server!";
    send(socket, msg, strlen(msg), 0);
    closesocket(socket);
    clientInfo->if_connected = 0;
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
                FD_SET(clientInfo[i].sCilent, &readSet);
            }
        }
        int maxSocket = (int)ListenSocket;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (clientInfo[i].if_connected) {
                if ((int)clientInfo[i].sCilent > maxSocket) {
                    maxSocket = (int)clientInfo[i].sCilent;
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
                        clientInfo[i].sCilent = clientSocket;
                        clientInfo[i].saClient = saClient;
                        clientInfo[i].if_connected = 1;
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
    }
    closesocket(ListenSocket);
    WSACleanup();
    exit(0);
}

#endif //LAB1_SERVER_H
