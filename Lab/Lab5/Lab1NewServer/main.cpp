#include <iostream>
#include "server.h"
#define MAX_NUM 100
int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed" << std::endl;
        return 1;
    }
    InitializeCriticalSection(&cs);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serAddr;
    memset(&serAddr, 0, sizeof(serAddr));
    serAddr.sin_port = htons(SERVER_PORT);
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sock, (sockaddr*)&serAddr, sizeof(serAddr));
    if (listen(sock, MAX_NUM) == 0) {
        // std::cout << "Listen successful" << std::endl;
    }
    else {
        std::cout << "Listen failed" << std::endl;
    }

    std::cout << "Waiting for client connecting!" << std::endl
        << "tips: Ctrl + C to quit!" << std::endl;

    while (1) {
        sockaddr_in cliAddr;
        int cliAddrLen = sizeof(cliAddr);
        SOCKET cliSock = accept(sock, (sockaddr*)&cliAddr, &cliAddrLen);
        ClientInfo cli;
        cli.port = ntohs(cliAddr.sin_port);
        cli.addr = inet_ntoa(cliAddr.sin_addr);
        cli.sClient = cliSock;
        cli.saClient = cliAddr;
        cli.if_connected = 1;
        clientList.push_back(cli);

        std::cout << "Client " << cli.addr << ":" << cli.port << " connected" << std::endl;
        HANDLE hThread;
        DWORD dwThreadId;

        const char *const welcomeMsg = "Hello, I'm server!";
        hThread = CreateThread(NULL, 0, HandleClientRequest, (LPVOID)cliSock, 0, &dwThreadId);
        send(cliSock, welcomeMsg, strlen(welcomeMsg), 0);
        CloseHandle(hThread);
    }

    WSACleanup();
    return 0;
}
