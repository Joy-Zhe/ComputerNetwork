#ifndef LAB1_PARSER_H
#define LAB1_PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
int connectedClients = 0; // connected clients

enum TYPE{ DONE = 0, CONNECT = 1, CANCEL = 2, TIME = 3, NAME = 4, CLIENT = 5, SEND = 6 };

struct Message{
    int type;
    char data[512];
};

//struct ClientList{
//    char ip[20];
//    char port[10];
//    char name[20];
//};
struct ClientList{
    char ip[20];
    unsigned short port;
    int if_connected;
    char name[20];
};

//struct responseBody{
//    int type;       // type
//    char msg[512];  // message for type 6
//    char ip[20];    // ip for type 1 and 2 and 4 and 5
//    char port[10];
//    char name[20];  // name for type 4
//    char time[20];
//    struct ClientList clientList[10];   // client list for type 5
//};

struct responseBody{
    int type;       // type
    char msg[512];  // message for type 6
    char ip[20];    // ip for type 1 and 2 and 4 and 5
    char port[10];
    char name[20];  // name for type 4
    char time[20];  // time for type 3
    struct ClientList clientList[10];   // client list for type 5
};



// request list
/*
 * actually the data part only will be used when type is 6
 * |type|data|
 * |0|Done|
 * |1|Connect|
 * |2|Cancel|
 * |3|LocalTime|
 * |4|HostName|
 * |5|List|
 * |6|<Message to be sent>|
 * <Message to be sent>: ip*port*content
 */

int ParseRequest(const char *requestStr, struct Message *message){
    //request format: "|type|data|"
    if (requestStr == NULL || message == NULL) {
        return -1;
    }
    char *typeStr = (char *)malloc(10 * sizeof(char));
    char requestCpy[512];
    strcpy(requestCpy, requestStr);
    typeStr = strtok(requestCpy, "|");
    if (typeStr == NULL) {
        printf("Wrong request format!\n");
        return -1;
    }
    message->type = atoi(typeStr);
    typeStr = strtok(NULL, "|");
    if (typeStr == NULL) {
        printf("Wrong request format!\n");
        return -1;
    }
    strcpy(message->data, typeStr);
    printf("Parsed request: type = %ld, data = %s\n", message->type, message->data);
    return message->type;
}

// use struct to package different information
// response list
void EncodeResponse(int type, const struct responseBody *data, char *responseStr){
    if (responseStr == NULL) {
        return;
    }
    char typeStr[10];
    sprintf(typeStr, "%d", type);
    strcpy(responseStr, "|");
    strcat(responseStr, typeStr);
    strcat(responseStr, "|");
//    strcat(responseStr, data);
//    strcat(responseStr, "|");
    switch (type) {
        case 0:
            printf("Done.\n");
            // for server
            break;
        case 1:
            strcat(responseStr, "Connect");
            strcat(responseStr, "|");
            strcat(responseStr, data->ip);
            strcat(responseStr, "|");
            strcat(responseStr, data->port);
            break;
        case 2:
            strcat(responseStr, "Cancel");
            strcat(responseStr, "|");
            strcat(responseStr, data->ip);
            strcat(responseStr, "|");
            strcat(responseStr, data->port);
            // for server
            break;
        case 3:
            strcat(responseStr, "LocalTime");
            strcat(responseStr, "|");
            strcat(responseStr, data->time);
            strcat(responseStr, "|");
            break;
        case 4:
            strcat(responseStr, "HostName");
            strcat(responseStr, "|");
            strcat(responseStr, data->name);
            strcat(responseStr, "|");
            break;
        case 5:
            strcat(responseStr, "List");
            strcat(responseStr, "|");
            // 构建客户端列表信息
            for (int i = 0; i < connectedClients; i++) {
                char clientInfo[512];
                sprintf(clientInfo, "\n|%d|%s|%s|", i + 1, data->clientList[i].ip, data->clientList[i].port);
                strcat(responseStr, clientInfo);
            }
            strcat(responseStr, "\n");
            break;
        case 6:
            strcat(responseStr, "Message");
            strcat(responseStr, "|");
            strcat(responseStr, data->msg);
            strcat(responseStr, "|");
            break;
        default:
            printf("Unknown type.\n");
            break;
    }
    printf("Encoded response: %s\n", responseStr);
}

#endif
