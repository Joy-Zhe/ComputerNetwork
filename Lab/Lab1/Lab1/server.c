#include "server.h"
#include "parser.h"
int main()
{
    // test parser
//    char *testResponse = (char *)malloc(256 * sizeof(char));
//    struct responseBody data;
//    strcpy(data.time, "2023-10-01 19:00:00");
//    struct Message message;
//    const char *requestStr = "|3|LocalTime|";
//    int type = ParseRequest(requestStr, &message);
//    printf("%d\n", type);
//    EncodeResponse(type, &data, testResponse);
    StartServer();
    return 0;
}