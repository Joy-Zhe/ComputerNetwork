#include <stdio.h>
#include <winsock2.h>
#define SERVER_PORT	4169 //listening port, my student ID

struct student
{
    char name[32];
    int age;
};

void main()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int ret, nLeft, length;
    SOCKET sListen, sServer; //listening socket，connection socket
    struct sockaddr_in saServer, saClient; // address info
    struct student stu;
    char *ptr;

    //initialize WinSock
    wVersionRequested = MAKEWORD(2, 2); // WinSock DLL version
    ret = WSAStartup( wVersionRequested, &wsaData ); // load WinSock DLL
    if (ret != 0)
    {
        printf("WSAStartup() failed!\n");
        return;
    }
    // confirm WinSock DLL version
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE( wsaData.wVersion ) != 2 )
    {
        WSACleanup();
        printf("Invalid WinSock version!\n");
        return;
    }

    // create listening socket
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sListen == INVALID_SOCKET)
    {
        WSACleanup();
        printf("socket() failed!\n");
        return;
    }

    saServer.sin_family = AF_INET; // address family
    saServer.sin_port = htons(SERVER_PORT); // listening port
    saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // listening address

    // bind
    ret = bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer));
    if (ret == SOCKET_ERROR)
    {
        printf("bind() failed! code:%d\n", WSAGetLastError());
        closesocket(sListen);// close socket
        WSACleanup();
        return;
    }

    // listen
    ret = listen(sListen, 5);
    if (ret == SOCKET_ERROR)
    {
        printf("listen() failed! code:%d\n", WSAGetLastError());
        closesocket(sListen);// close socket
        WSACleanup();
        return;
    }

    printf("Waiting for client connecting!\n");
    printf("tips : Ctrl+c to quit!\n");
    //阻塞等待接受客户端连接：
    length = sizeof(saClient);
    sServer = accept(sListen, (struct sockaddr *)&saClient, &length);
    if(sServer == INVALID_SOCKET)
    {
        printf("accept() failed! code:%d\n", WSAGetLastError());
        closesocket(sListen);// close socket
        WSACleanup();
        return;
    }
    printf("Accepted client: %s:%d\n",
           inet_ntoa(saClient.sin_addr), ntohs(saClient.sin_port));

    //按照预定协议，客户端将发来一个学生的信息：
    nLeft = sizeof(stu);
    ptr = (char *)&stu;
    while(nLeft >0)
    {
        //接收数据：
        ret = recv(sServer, ptr, nLeft, 0);
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
        ptr += ret;
    }

    if(!nLeft) // all data received
        printf("name: %s\nage:%d\n", stu.name, stu.age);

    closesocket(sListen);// close socket
    closesocket(sServer);
    WSACleanup();
}