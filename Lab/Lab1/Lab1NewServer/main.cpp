#include <iostream>
#include "server.h"
#define MAX_NUM 1000
int main() {
//    std::std::cout << "Hello, World!" << std::std::endl;
//    return 0;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

//    u_long nonBlockingMode = 1;
//    if (ioctlsocket(sock, FIONBIO, &nonBlockingMode) == SOCKET_ERROR) {
//        std::cerr << "Failed to set socket to non-blocking mode." << std::endl;
//        closesocket(sock);
//        WSACleanup();
//        return 1;
//    }

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

    std::cout << "Server is listening and ready to connect" << std::endl;

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

        hThread = CreateThread(NULL, 0, HandleClientRequest, (LPVOID)cliSock, 0, &dwThreadId);
        CloseHandle(hThread);
    }

    WSACleanup();
    return 0;
}
