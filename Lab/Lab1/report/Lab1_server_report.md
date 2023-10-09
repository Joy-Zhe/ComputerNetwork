## 各函数实现的功能

### Server 网络连接部分

> 方法位于`server.h`

1. **UpdateMaxIndex() 函数：**

    - **功能：** 此函数用于更新连接到服务器的客户端数量的最大索引。
    - **原理：** 它比较已连接客户端的数量 `connectedClients` 和最大索引 `maxIndex`，并将 `maxIndex` 更新为更大的值，以确保能够正确地索引所有已连接的客户端。

2. **InitWinSocket() 函数：**
    - **功能：** 初始化 Winsock2 库，准备进行网络通信。
    - **原理：** 使用 `WSAStartup` 函数初始化 Winsock2，并检查所使用的 Winsock 版本是否正确。

```C
void InitWinSocket(){
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup() failed!\n");
		exit(1);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Invalid WinSock version!\n");
		WSACleanup();
		exit(1);
	}
}
```

3. **CreateListeningSocket() 函数：**

    - **功能：** 创建服务器监听套接字。
    - **原理：** 使用 `socket` 函数创建一个套接字，并初始化 `saServer` 结构以指定监听的 IP 地址和端口。

``` C
void CreateListeningSocket(SOCKET *ListenSocket, struct sockaddr_in *saServer) {  
	struct addrinfo *result = NULL, *ptr = NULL, hints;  
	hints.ai_family = AF_INET;  
	hints.ai_socktype = SOCK_STREAM;  
	hints.ai_protocol = IPPROTO_TCP;  
	hints.ai_flags = AI_PASSIVE; // for wildcard IP address  
	  
	*ListenSocket = INVALID_SOCKET;  
	*ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
	if(*ListenSocket == INVALID_SOCKET)  
	{  
		WSACleanup();  
		printf("socket() failed!\n");  
		exit(1);  
	}  
	  
	(*saServer).sin_family = AF_INET; // address family  
	(*saServer).sin_port = htons(SERVER_PORT); // listening port  
	(*saServer).sin_addr.S_un.S_addr = htonl(INADDR_ANY); // listening address  
}
```

4. **BindSocket() 函数：**

    - **功能：** 将套接字绑定到指定的 IP 地址和端口。
    - **原理：** 使用 `bind` 函数将监听套接字与指定的 IP 地址和端口绑定。

```

```

4. **Listen() 函数：**

    - **功能：** 启动监听，等待客户端连接。
    - **原理：** 使用 `listen` 函数启动监听套接字，以便接受客户端连接请求。

5. **BroadcastMsg() 函数：**

    - **功能：** 将消息广播给指定的客户端。
    - **原理：** 遍历已连接的客户端列表，查找与指定 IP 和端口匹配的客户端，并将消息发送给匹配的客户端。

6. **HandleClient() 函数：**

    - **功能：** 处理单个客户端连接的线程函数。
    - **原理：** 此函数在一个独立的线程中运行，用于处理客户端的连接。它接收客户端发来的消息，解析消息内容，并根据消息类型执行相应的操作。如果客户端关闭连接或发生错误，该函数将关闭套接字并终止线程。

7. **StartServer() 函数：**

    - **功能：** 启动服务器，接受客户端连接并处理消息传递。
    - **原理：** 此函数包括初始化 Winsock2，创建监听套接字，开始监听，然后进入主循环以接受和处理客户端连接。
