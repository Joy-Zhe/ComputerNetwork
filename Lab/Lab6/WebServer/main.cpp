#include "webServer.h"

int main() {
    WebServer server(SOCKET_PORT);
    server.Start();

    return 0;
}