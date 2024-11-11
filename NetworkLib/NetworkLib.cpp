#include "NetworkLib.h"

void InitializeConnection(Connection* connection) {
    connection->socket = INVALID_SOCKET;
    memset(&connection->address, 0, sizeof(connection->address));  // Zero out the address structure
}

int InitializeWindowsSockets()
{
    WSADATA wsaData;

    // Initialize windows sockets library for this process
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        PrintSocketError("WSAStartup failed");

        return iResult;
    }

    return 0;
}

int CreateServerSocket(Connection* connection, const char* port)
{
    int iResult;

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
    if (iResult != 0) {
        PrintSocketError("getaddrinfo failed");

        return iResult;
    }

    // Create a SOCKET for listening on server
    connection->socket = socket(resultingAddress->ai_family, resultingAddress->ai_socktype, resultingAddress->ai_protocol);
	if (connection->socket == INVALID_SOCKET) {
        PrintSocketError("socket failed");
        freeaddrinfo(resultingAddress);

        return connection->socket;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    iResult = bind(connection->socket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("bind failed");
        freeaddrinfo(resultingAddress);
        closesocket(connection->socket);

        return iResult;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set socket in listening mode
    iResult = listen(connection->socket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("listen failed");
        closesocket(connection->socket);

        return iResult;
    }

	return 0;
}

int CreateClientSocket(Connection* connection, const char* ipAddress, const unsigned short port)
{
    int iResult;

    // Create a socket for connecting to the server
    connection->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connection->socket == INVALID_SOCKET) {
        PrintSocketError("socket failed");

        return connection->socket;
    }

    // Specify server address using sockaddr_in
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress);   // Use the IP passed in, e.g., "127.0.0.1"
    serverAddress.sin_port = htons(port);                   // Convert the port string to an integer

    // Connect to server
    iResult = connect(connection->socket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("connect failed");
        closesocket(connection->socket);

        return iResult;
    }

    return 0;
}

int AcceptConnection(Connection* serverConnection, Connection* clientConnection)
{
    int addressSize = sizeof(clientConnection->address);

    clientConnection->socket = accept(serverConnection->socket, (struct sockaddr*)&clientConnection->address, &addressSize);
    if (clientConnection->socket == INVALID_SOCKET) {
        PrintSocketError("accept failed");

        return clientConnection->socket;
    }

    return 0;
}

int ReceiveData(Connection* connection, char* buffer, int bufferLength)
{
    int iResult = recv(connection->socket, buffer, bufferLength, 0);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("recv failed");

        return iResult;
	}

    return iResult;
}

int SendData(Connection* connection, const char* data, int length)
{
    int iResult = send(connection->socket, data, length, 0);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("send failed");

        return iResult;
    }

    return iResult;
}

void CloseConnection(Connection* connection)
{
    int iResult = closesocket(connection->socket);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("Closesocket failed");
    }
}

void ShutdownServer(Connection* serverConnection, Connection clientConnections[], int clientCount) {
    int iResult;

    // Stop accepting new connections on the server socket
    if (serverConnection->socket != INVALID_SOCKET) {
       iResult = shutdown(serverConnection->socket, SD_BOTH);
        if (iResult == SOCKET_ERROR)
        {
            PrintSocketError("shutdown failed");
        }

        CloseConnection(serverConnection);
        serverConnection->socket = INVALID_SOCKET;
    }

    // Close each active client connection
    for (int i = 0; i < clientCount; i++) {
        if (clientConnections[i].socket != INVALID_SOCKET) {
            iResult = shutdown(clientConnections[i].socket, SD_BOTH);
            if (iResult == SOCKET_ERROR)
            {
                PrintSocketError("shutdown failed");
            }

            CloseConnection(&clientConnections[i]);
            clientConnections[i].socket = INVALID_SOCKET;
        }
    }
}

void PrintSocketError(const char* message)
{
    fprintf(stderr, "%s with error: %d\n", message, WSAGetLastError());
}
