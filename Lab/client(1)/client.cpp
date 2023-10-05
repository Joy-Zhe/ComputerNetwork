#include "parser.h"
#include <stdio.h>
#include <WinSock2.h>
#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")
#define USER_OP_CONNECT       "c"
#define USER_OP_BREAKCONN     "b"
#define USER_OP_TIME		  "t"
#define USER_OP_NAME          "n"
#define USER_OP_LIST          "l"
#define USER_OP_SEND          "s"
#define USER_OP_QUIT          "q"
#define USER_OP_HELP          "h"
#define HELP_INFORMATION "HELP:\n \
		[1] Connet to Server               ==>  c\n \
		[2] Get server time                ==>  t\n \
		[3] Get server Name                ==>  n\n \
		[4] Get clients lists              ==>  l\n \
		[5] Send Messege to other clients  ==>  s\n \
		[6] Disconnet from Server          ==>  b\n \
		[7] Quit the program               ==>  q\n \
		[8] Get help                       ==>  h\n"
#define MAXCONTENT         512
/*------- GLOBAL VARIABLE REGION -------*/
BOOL CONNECTION_STATUS = FALSE;
//Global HANDLE for user input
HANDLE hEvent_User = NULL;
//Global HANDLE for checking Receive_Pipeline
HANDLE hEvent_Receiving = NULL;
//Global recvBUFFER
char recvBuf[2048] = { 0 };
//Global this->SOCKET, each connection will have a new one and disconn. to close that
SOCKET Client_SOCKET = 0;


void PktRequest(struct Message* t, char* s){
    strcpy(s, "\0");           //clean s
    strcpy(s, "|");            //"|" means a split character
    char type_[2];
    memset(type_, '\0', sizeof(type_));
    sprintf(type_, "%d", t->type);
    strcat(s, type_);        //request type
    strcat(s, "|");
    /* |type| */
    strcat(s, t->data);
    strcat(s, "|");
    /* |type|data| */
}

void getTime(){
    struct Message* temp = (struct Message*)malloc(sizeof(struct Message));
    char* s = (char*)malloc(sizeof(char));
    //strcpy(temp->type, "time");
    temp->type = TIME;
    PktRequest(temp, s);
    int ret = send(Client_SOCKET, s, strlen(s), 0);
    if (ret == SOCKET_ERROR) printf("getTime send() failed!\n");
}

void getName(){
    struct Message* temp = (struct Message*)malloc(sizeof(struct Message));
    char* s = (char*)malloc(sizeof(char));
    //strcpy(temp->type, "time");
    temp->type = NAME;
    PktRequest(temp, s);
    int ret = send(Client_SOCKET, s, strlen(s), 0);
    if (ret == SOCKET_ERROR) printf("getName send() failed!\n");
}

void getClientList( ) {
    struct Message* temp = (struct Message*)malloc(sizeof(struct Message));
    //strcpy(temp->type, "list");
    temp->type = CLIENT;
    char* s = (char*)malloc(sizeof(char));
    PktRequest(temp, s);
    int ret = send(Client_SOCKET, s, strlen(s), 0);
    if (ret == SOCKET_ERROR) printf("getClientList send() failed!\n");
}

void DisconnetfromServer(){
    CONNECTION_STATUS = FALSE;
    //Close sub-threads
    //CloseHandle
    struct Message* temp = (struct Message*)malloc(sizeof(struct Message));
    char* s = (char*)malloc(sizeof(char));
    //strcpy(temp->type, "quit");
    temp->type = CANCEL;
    PktRequest(temp, s);
    int ret = send(Client_SOCKET, s, strlen(s), 0);
    if (ret == SOCKET_ERROR) printf("DisconnetformServer send() failed!\n");
}
/*data=<Message to be sent>: ip*port*content*/
void sendMessageForward( )
{
    struct Message* temp = (struct Message*)malloc(sizeof(struct Message));
    //strcpy(temp->type, "msg");
    temp->type = SEND;

    printf("input your target ip: ");
    char ip[20];
    memset(ip, '\0', sizeof(ip));
    scanf("%s", ip);
    getchar(); //get rid of the '\n'

    printf("input your target port: ");
    char port[5];
    memset(port, '\0', sizeof(port));
    scanf("%s", port);
    getchar(); //get rid of the '\n'

    printf("input your target msg:  ");
    char content[MAXCONTENT];
    memset(content,'\0',sizeof(content));
    fgets(content, MAXCONTENT, stdin);

    char* s = (char*)malloc(sizeof(char));
    //PktRequest(temp, s);
    strcpy(s, "\0");           //clean s
    strcpy(s, "|");            //"|" means a split character
    char type_[2];
    memset(type_, '\0', sizeof(type_));
    sprintf(type_, "%d", temp->type);
    strcat(s, type_);        //request type
    strcat(s, "|");
    /* |type| */
    strcat(s, ip);
    strcat(s, "*");
    strcat(s, port);
    strcat(s, "*");
    strcat(s, content);
    strcat(s, "*");
    strcat(s, "|");
    /* |type|data| */
    /* data : ip*port*content*/
    int ret = send(Client_SOCKET, s, strlen(s), 0);
    if (ret == SOCKET_ERROR) printf("sendMessageForward send() failed!\n");
}

void QuitProgram() {
    if (CONNECTION_STATUS == TRUE) DisconnetfromServer();
    exit(0);
}

SOCKET Socket_Systemcall() {
    SOCKET mSOCKET = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mSOCKET == INVALID_SOCKET){
        WSACleanup();
        printf("socket() failed!\n");
        return 0; //TODO
    }
    printf("SOCKET: %lu\n", mSOCKET);
    //Create User_Pipeline's eventMsg ( , mannul, reset , init , )
    hEvent_User = CreateEvent(NULL, TRUE, FALSE, NULL);
    return mSOCKET;
}

void UnPackageMsg(char* s, struct responseBody *t){
    /*|type|data|*/
    char data[111];//MAXCONTENT
    char type_[20];
    int type;
    memset(data, '\0', sizeof(data));
    memset(type_,'\0',sizeof(type_));
    sscanf(s, "|%d|%s|%s|", &type,type_,data);
    switch (type)
    {
    case TYPE::NAME:
        strcpy(t->name, data);
        break;
    case TYPE::TIME:
        strcpy(t->time, data);
        break;
    case TYPE::SEND:
        strcpy(t->msg, data);
        break;
    case TYPE::CANCEL:
        break;
    case TYPE::CONNECT:
        break;
    case TYPE::CLIENT:
    // char clientInfo[512];
    // sprintf(clientInfo, "\n|%d|%s|%s|", i + 1,
    // data->clientList[i].ip, data->clientList[i].port);
        {
            char ip[20];
            char port[20];
            int client_id = 0;
            for(int i = 0; i < connectedClients; i++)
            {
                memset(ip, '\0', sizeof(ip));memset(port, '\0', sizeof(port));
                sscanf(s,"\n|%d|%s|%s|",&client_id,ip,port);
                strcpy((t->clientList[i]).ip, ip);
                strcpy((t->clientList[i]).port, port);
            }
        }
        break;
    default:
        break;
    }
}

DWORD WINAPI Receive_Pipeline(LPVOID lpParameter) {
    printf("SubThread_Receiving ID:%4d created!\n", GetCurrentThreadId());
//    printf("-----------------------------\n");
    int ret = 0;

    //GetMessage, PostThreadMessage, PostMessage
    //select fd_set
    memset(recvBuf, 0, sizeof(recvBuf)); //the Global: char *recvBuf
    while ( true ){

        ret = recv(Client_SOCKET, recvBuf, sizeof(recvBuf), 0);
        if (ret <= 0){
            printf("recv failed!\n");
            break;
        }else{
            printf("recv succeed\n");
            printf("recv content is: %s\n",recvBuf);
        }

        struct responseBody* temp = (struct responseBody*)malloc(sizeof(struct responseBody));
        UnPackageMsg(recvBuf, temp);
        if (temp->type == TYPE::DONE){
            printf("Disconnected\n");
            break;
        }else if(temp->type == TYPE::NAME){
            strcpy(recvBuf,temp->name);
            printf("host name is %s", temp->name);
        }else if(temp->type == TYPE::TIME){
            strcpy(recvBuf,temp->time);
            printf("host time is %s", temp->time);
        }else if(temp->type == TYPE::SEND){
            strcpy(recvBuf,temp->msg);
        }else if(temp->type == TYPE::CANCEL){
            printf("Disconnected\n");
            break;
        }else if(temp->type == TYPE::CLIENT ){
            for(int i = 0; i < connectedClients; i++){
                struct ClientList cli = temp->clientList[i];
                printf("client ip:%s port:%s\n",cli.ip ,cli.port);// name:%s,cli.name
            }
        }
        memset(recvBuf, 0, sizeof(recvBuf)); //the Global: char *recvBuf
        SetEvent(hEvent_Receiving);
    }

    printf("SubThread ID:%d stop!\n", GetCurrentThreadId());
//    memset(recvBuf, 0, sizeof(recvBuf)); //the Global: char *recvBuf
    closesocket(Client_SOCKET);

    return 1;
}
void Connect2Server( ) {

    u_long _serverPort = 0;
    char  _serverIP[64] = { 0 };
    printf("Please type in your target server's IP:  ");
    scanf("%s", _serverIP);
    printf("Please type in your target server's Port:  ");
    scanf("%lu", &_serverPort);

    //服务器地址信息对象
    SOCKADDR_IN saServer;
    //地址家族
    saServer.sin_family = AF_INET;
    //服务器端口号
    saServer.sin_port = htons(_serverPort); //"Host to Network Long"
    //服务器IP
    saServer.sin_addr.s_addr = inet_addr(_serverIP);

    Client_SOCKET = Socket_Systemcall();

    int ret = connect(Client_SOCKET, (PSOCKADDR)&saServer, sizeof(saServer));
    if (ret == SOCKET_ERROR){
        printf("connect() failed!\n");
        closesocket(Client_SOCKET);  //关闭套接字
        WSACleanup();
        return;
    }
    //recvmsg();
    //sendmsg();

    CONNECTION_STATUS = TRUE;

    //Create Receive_Pipeline's eventMsg ( , mannul, reset , init , )
    hEvent_Receiving = CreateEvent(NULL, TRUE, FALSE, NULL);

    //Create sub-threads
    //CreateSubThread_Receiving( );
    HANDLE hThread = NULL;
    DWORD  threadId = 0;
    hThread = CreateThread(NULL, 0, Receive_Pipeline, NULL, 0, &threadId);
    if (hThread == NULL || threadId == 0) printf("CreatThread failed.\n");
}

DWORD WINAPI User_Pipeline(LPVOID lpParameter) {
    printf("SubThread_userPipeline ID:%4d created!\n", GetCurrentThreadId());
    while (true) {
        printf("order type is:\n");
        char input_order[3];
        memset(input_order, '\0', sizeof(input_order));
        scanf("%s",input_order);
        if(strcmp(input_order,USER_OP_CONNECT)==0){
            Connect2Server();
        }else if(strcmp(input_order,USER_OP_TIME)==0){
            getTime();
        }else if(strcmp(input_order,USER_OP_NAME)==0){
            getName();
        }else if(strcmp(input_order,USER_OP_LIST)==0){
            getClientList();
        }else if(strcmp(input_order,USER_OP_SEND)==0){
            sendMessageForward();
        }else if(strcmp(input_order,USER_OP_BREAKCONN)==0){
            DisconnetfromServer();
        }else if(strcmp(input_order,USER_OP_QUIT)==0){
            QuitProgram();
        }else if(strcmp(input_order,USER_OP_HELP)==0){
            printf(HELP_INFORMATION);
        }else{
            printf("invalid input order : %s with length %d\n", input_order, strlen(input_order));
        }
        SetEvent(hEvent_User);
    }
}

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);  //希望使用的WinSock DLL的版本
    if (WSAStartup(wVersionRequested, &wsaData) != 0){
        printf("WSAStartup() failed!\n");
        return 1;
    }
    //确认WinSock DLL支持版本2.2：
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
        WSACleanup();
        printf("Invalid Winsock version!\n");
        return 2;
    }
    //CreateSubThread_User();
    HANDLE hThread = NULL;
    DWORD  threadId = 0;
    hThread = CreateThread(NULL, 0, User_Pipeline, NULL, 0, &threadId);
    if (hThread == NULL || threadId == 0){
        printf("CreatThread failed.\n");
    }
    //GetHelp();
    printf("HELP:\n \
		[1] Connet to Server               ==>  c\n\
		[2] Get server time                ==>  t\n\
		[3] Get server Name                ==>  n\n\
		[4] Get clients lists              ==>  l\n\
		[5] Send Messege to other clients  ==>  s\n\
		[6] Disconnet from Server          ==>  b\n\
		[7] Quit the program               ==>  q\n\
		[8] Get help                       ==>  h\n");
    printf("-----------------------------\n");
    while (true){
        if (WaitForSingleObject(hEvent_User, 10) != WAIT_OBJECT_0){}  //NO user input
        else{ //with user input
            ResetEvent(hEvent_User);
            Sleep(1);
        }
        if (WaitForSingleObject(hEvent_Receiving, 10) != WAIT_OBJECT_0){} //NO receiving data
        else{  //with receiving data
            ResetEvent(hEvent_Receiving);
            printf("%s\n", recvBuf);
            printf("-----------------------------\n");
            Sleep(1);
        }
    }
    return 0;
}

