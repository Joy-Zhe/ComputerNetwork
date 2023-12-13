#ifndef WEBSERVER_WEBSERVER_H
#define WEBSERVER_WEBSERVER_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <process.h>
#include <winsock2.h>
#define SOCKET_PORT 4169

#pragma comment(lib, "ws2_32.lib")

class WebServer {
public:
    explicit WebServer(int port) : port(port), serverSocket(0) {}

    ~WebServer() {
        Stop();
    }

    void Start() {
        Initialize();
        std::cout << "Server started on port " << port << std::endl;

        while (!shouldStop) {
            SOCKET clientSocket = AcceptConnection();
            if (clientSocket != INVALID_SOCKET) {
                auto *param = new ThreadParam{this, clientSocket}; // Pass the pointer to the ThreadParam struct
                CreateThread(nullptr, 0, HandleClient, reinterpret_cast<LPVOID>(param), 0, nullptr);
            }
        }

        std::cout << "Server stopped" << std::endl;
    }

    void Stop() {
        shouldStop = true;
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
        }
        cv.notify_all();
        WSACleanup();
    }

private:
    int port;
    SOCKET serverSocket;
    bool shouldStop = false;
    std::mutex mtx;
    std::condition_variable cv;

    struct ThreadParam {
        WebServer* webServer;
        SOCKET clientSocket;
    };

    void Initialize() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            perror("Error initializing SOCKET");
            std::exit(EXIT_FAILURE);
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            perror("Error creating SOCKET");
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            perror("Error binding socket");
            std::exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, 10) == SOCKET_ERROR) {
            perror("Error listening on socket");
            std::exit(EXIT_FAILURE);
        }
    }

    [[nodiscard]] SOCKET AcceptConnection() const {
        sockaddr_in clientAddress{};
        int clientAddrLen = sizeof(clientAddress);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            perror("Error accepting connection");
            return INVALID_SOCKET;
        }

//        std::cout << "Client connected: " << inet_ntoa(clientAddress.sin_addr) << std::endl;
        return clientSocket;
    }

    static DWORD WINAPI HandleClient(LPVOID param) {
        char buffer[1024 * 1024] = {0};
        // reload the param
        auto *threadParam = static_cast<ThreadParam*>(param); //ThreadParam*
        auto *webServerParam = static_cast<WebServer*>(threadParam->webServer);
        SOCKET clientSocket = threadParam->clientSocket;
        struct sockaddr_in HTTP_addr = {0};
        int HTTP_addr_len = sizeof(HTTP_addr);
        if (getpeername(clientSocket, (struct sockaddr *)&HTTP_addr, &HTTP_addr_len) == SOCKET_ERROR) {
            perror("Error getting client address");
            closesocket(clientSocket);
            return -1;
        }
        std::cout << "Client connected: " << inet_ntoa(HTTP_addr.sin_addr) << ":" << HTTP_addr.sin_port << std::endl;

        while (true) {
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesRead <= 0) {
                perror("Error reading from client");
                closesocket(clientSocket);
                return -1;
            }

            std::string request(buffer);

//            std::cout << "Request: " << std::endl << request << std::endl;
            std::istringstream iss(request);
            std::string method, file, headers;
            iss >> method >> file >> headers;

//            std::cout << "Method: " << method << std::endl;
//            std::cout << "File: " << file << std::endl;
//            std::cout << "Headers: " << headers << std::endl;

            if (method == "GET") {
                webServerParam->HandleGet(clientSocket, file);
            } else if (method == "POST") {
                webServerParam->HandlePost(clientSocket, file, request);
            } else {
                webServerParam->SendNotFoundResponse(clientSocket);
            }
        }

        closesocket(clientSocket);
        return 0;
    }

    void HandleGet(SOCKET clientSocket, const std::string& url) {
//        std::cout << "Handling GET request" << std::endl;

        std::string type;
        std::string name = ".";

        if (url.find(".txt") != std::string::npos) {
            type = "text/plain";
            name += "/txt";
        } else if (url.find(".html") != std::string::npos) {
            name += "/html";
            type = "text/html";
        } else if (url.find(".png") != std::string::npos) {
            type = "image/png";
            name += "/img";
        } else if (url.find(".ico") != std::string::npos) {
            type = "image/ico";
            name += "/img";
        }

        name += url;
//        std::cout << "Finding the file in " << name << std::endl;

        std::ifstream fileStream(name, std::ios::binary);

        if (fileStream.is_open()) {
//            std::cout << "File found" << std::endl;

            fileStream.seekg(0, std::ios::end);
            std::streampos filelen = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);

            std::ostringstream contentStream;
            contentStream << fileStream.rdbuf();
            std::string content = contentStream.str();

            char len[10];
            _ltoa_s(static_cast<long>(filelen), len, 10);

            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Connection: keep-alive\r\n";
            response += "Content-Length:" + std::string(len) + "\r\n";
            response += "Server: csr_http1.1\n";

            if (url.find(".png") != std::string::npos) {
                response += "Accept-Ranges: bytes\r\n";
            }

            response += "Content-Type: " + type + "\r\n";
            response += "\r\n" + content;

            SendHttpResponse(clientSocket, response);
//            std::cout << "Send successfully" << std::endl;
        } else {
//            std::cout << "No such file existed: " << name << std::endl;

            std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\n";
            notFoundResponse += "Content-Type: text/html;charset=utf-8\r\n";
            notFoundResponse += "Content-Length: 84\r\n\r\n";
            notFoundResponse += "<html><body><h1>404 Not Found</h1><p>From server: URL is wrong.</p></body></html>\r\n";

            SendHttpResponse(clientSocket, notFoundResponse);
//            std::cout << "Couldn't find message send error" << std::endl;
        }
    }

    void HandlePost(SOCKET clientSocket, const std::string& file, const std::string& request) {
        std::cout << "Handling POST request" << std::endl;
        std::string successMsg = "<html><body><h1>Login Success</h1><p>From server: Login successfully.</p></body></html>\r\n";
        std::string failedMsg = "<html><body><h1>Login Failed</h1><p>From server: Login failed.</p></body></html>\r\n";
        // parse "login=xxx&pass=xxx"
        std::string login = request.substr(request.find("login=") + strlen("login="));
        login = login.substr(0, login.find("&"));
        std::string password = request.substr(request.find("pass=") + strlen("pass="));
        password = password.substr(0, password.find("&"));

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: ";

        if (login == "3210104169" && password == "4169") {
            response += std::to_string(successMsg.size());
            response += "\r\n\r\n";
            response += successMsg;
            std::cout << "Login successfully" << std::endl;
        } else {
            response += std::to_string(failedMsg.size());
            response += "\r\n\r\n";
            response += failedMsg;
            std::cout << "Login failed" << std::endl;
        }

        SendHttpResponse(clientSocket, response);
    }

    static void SendHttpResponse(SOCKET clientSocket, const std::string& response) {
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    static void SendOkResponse(SOCKET clientSocket) {
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: 0\r\n";
        response += "\r\n";

        send(clientSocket, response.c_str(), response.size(), 0);
    }

    static void SendNotFoundResponse(SOCKET clientSocket) {
        std::string response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: 0\r\n";
        response += "\r\n";

        send(clientSocket, response.c_str(), response.size(), 0);
    }
};


#endif //WEBSERVER_WEBSERVER_H
