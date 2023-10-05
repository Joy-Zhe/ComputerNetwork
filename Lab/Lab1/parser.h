#ifndef LAB1_PARSER_H
#define LAB1_PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int connectedClients = 0; // connected clients

enum TYPE{ DONE = 0, CONNECT = 1, CANCEL = 2, TIME = 3, NAME = 4, CLIENT = 5, SEND = 6 };

struct Message{
    int type;
    char data[512];
};

struct ClientInfo {
    SOCKET sClient;
    struct sockaddr_in saClient;
    int if_connected;
};
struct ClientInfo clientList[FD_SETSIZE];

//struct ClientList{
//    char ip[20];
//    unsigned short port;
//    int if_connected;
//    char name[20];
//};
//struct ClientList clientList[FD_SETSIZE]; // client list

struct responseBody{
    int type;       // type
    char msg[512];  // message for type 6
    char ip[20];    // ip for type 1 and 2 and 4 and 5
    char port[10];
    char name[20];  // name for type 4
    char time[20];
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
    printf("Parsed request: type = %d, data = %s\n", message->type, message->data);
    return message->type;
}

// for Send Message to other clients
void DecodeInfo(const char *msg, struct responseBody *data) {
    if (msg == NULL || data == NULL) {
        return;
    }
    char *infoStr = (char *)malloc(512 * sizeof(char));
    strcpy(infoStr, msg);
    char *ipStr = strtok(infoStr, "*");
    if (ipStr == NULL) {
        printf("Wrong request format!\n");
        return;
    }
    strcpy(data->ip, ipStr);
    ipStr = strtok(NULL, "*");
    if (ipStr == NULL) {
        printf("Wrong request format!\n");
        return;
    }
    strcpy(data->port, ipStr);
    ipStr = strtok(NULL, "*");
    if (ipStr == NULL) {
        printf("Wrong request format!\n");
        return;
    }
    strcpy(data->msg, ipStr);
    printf("Decoded info: ip = %s, port = %s, msg = %s\n", data->ip, data->port, data->msg);
}

// use struct to package different information
// response list
void EncodeResponse(int type, const struct responseBody *data/*, const char *data*/, char *responseStr){
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
            // construct client list info
            char clientNum[10];
            sprintf(clientNum, "%d", connectedClients);
            strcat(responseStr, clientNum);
            strcat(responseStr, "|");

            for (int i = 1; i <= connectedClients; i++) {
                char clientInfo[512];
                char cur_ip[20];
                unsigned short cur_port = ntohs(clientList[i - 1].saClient.sin_port);
                strcpy(cur_ip, inet_ntoa(clientList[i - 1].saClient.sin_addr));
                sprintf(clientInfo, "\n|%d|%s|%d|", i, cur_ip, cur_port);
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
