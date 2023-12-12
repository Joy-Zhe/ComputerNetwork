#include "server.h"
#define SERVER_PORT 2044
#define MAX_NUM 5

int main ( ) {
    g_Mutex = CreateMutex(NULL,false,NULL);  // 创建互斥量，初始化为触发状态
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed" << std::endl;
        return 1;
    }
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

    std::cout << "Server is listening and ready to connect" << std::endl;

    SOCKET sClient;
    sockaddr_in cliAddr;
    int nAddrlen = sizeof(cliAddr);
    while (true)
    {
        cout << "waiting for connection" << endl;
        // 从队列中取得一个新的请求建立连接
        sClient = accept(sock, (SOCKADDR*)&cliAddr, &nAddrlen);
        if (sClient == INVALID_SOCKET)
        {
            cout << "accept error !" << endl;
            continue;
        }
        cout << "accept a new connection" << endl;
        // save the client info
        ClientInfo cli;
        cli.port = ntohs(cliAddr.sin_port);
        cli.addr = inet_ntoa(cliAddr.sin_addr);
        cli.sClient = sClient;
        cli.saClient = cliAddr;
        cli.if_connected = 1;
        clientList.push_back(cli);

        std::cout << "Client " << cli.addr << ":" << cli.port << " connected" << std::endl;
        // 创建新的Client
//        Client* newClient = new Client(sClient, remoteAddr.sin_port, inet_ntoa(remoteAddr.sin_addr));
//        clients.push_back(*newClient);
        CreateThreadToHandleRequest(sClient);
    }
    closesocket(sock);
    CloseHandle(g_Mutex);
    WSACleanup();
}