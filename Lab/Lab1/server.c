#include "server.h"

int main()
{
//    int ret, nLeft, length;
//    SOCKET sListen, sServer; //listening socket，connection socket
//    struct sockaddr_in saServer, saClient; // address info
//    struct student stu;
//    char *ptr;

    //initialize WinSock
//    InitWinSocket();
//
//    // create listening socket
//    CreateListeningSocket(&sListen, &saServer);
//
//    // bind
//    BindSocket(&sListen, &saServer);
//
//    // listen
//    Listen(&sListen);
//
//    printf("Waiting for client connecting!\n");
//    printf("tips : Ctrl+c to quit!\n");
//    //阻塞等待接受客户端连接：
//    WaitForConnection(&sListen, &sServer, &saClient);
//
//    // information
//    Interaction(stu, &sServer, &sListen, &ptr);
    StartServer();
    return 0;
}