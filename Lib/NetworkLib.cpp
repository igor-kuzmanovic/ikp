#include "NetworkLib.h"

static void PrintSocketError(const char* message) {
    PrintError("%s with error %d.", message, WSAGetLastError());
}

void InitializeConnection(Connection* connection) {
    connection->socket = INVALID_SOCKET;
    // Zero out the address structure
    memset(&connection->address, 0, sizeof(connection->address));
}

int InitializeWindowsSockets() {
    WSADATA wsaData;

    // Initialize windows sockets library for this process
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        PrintSocketError("WSAStartup failed");

        return iResult;
    }

    return 0;
}

int CreateServerSocket(Connection* connection, const char* port) {
    int iResult;

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;

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

        return -1;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    iResult = bind(connection->socket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("bind failed");
        freeaddrinfo(resultingAddress);
        CloseConnection(connection);

        return iResult;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set socket in listening mode
    iResult = listen(connection->socket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("listen failed");
        CloseConnection(connection);

        return iResult;
    }

    return 0;
}

int CreateClientSocket(Connection* connection, const char* ipAddress, const char* port) {
    int iResult;

    // Create a socket for connecting to the server
    connection->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connection->socket == INVALID_SOCKET) {
        PrintSocketError("socket failed");

        return -1;
    }

    // Specify server address using sockaddr_in
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress);
    serverAddress.sin_port = htons(atoi(port));

    // Connect to server
    iResult = connect(connection->socket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("connect failed");
        CloseConnection(connection);

        return iResult;
    }

    // Sets the socket into non-blocking mode
    u_long mode = 1;
    iResult = ioctlsocket(connection->socket, FIONBIO, &mode);
    if (iResult != NO_ERROR) {
        PrintSocketError("ioctlsocket failed");
        CloseConnection(connection);

        return iResult;
    }

    return 0;
}

int AcceptConnection(Connection* serverConnection, Connection* clientConnection) {
    int addressSize = sizeof(clientConnection->address);

    clientConnection->socket = accept(serverConnection->socket, (struct sockaddr*)&clientConnection->address, &addressSize);
    if (clientConnection->socket == INVALID_SOCKET) {
        PrintSocketError("accept failed");

        return -1;
    }

    return 0;
}

int ReceiveData(Connection* connection, char* buffer, int bufferLength) {
    int iResult;
    fd_set readSet{};
    struct timeval timeout {};
    int lastError;

    // Set a timeout
    timeout.tv_sec = 0;  // Timeout in seconds
    timeout.tv_usec = 0; // Timeout in microseconds

    while (true) {
        // Initialize the fd_set for the socket
        FD_ZERO(&readSet);
        FD_SET(connection->socket, &readSet);

        // Wait for the socket to become readable
        iResult = select(0, &readSet, NULL, NULL, &timeout);
        if (iResult == SOCKET_ERROR) {
            PrintSocketError("select failed");

            return iResult;
        }
        else if (iResult == 0) {
            // Timeout occurred

            continue;
        }
        else {
            PrintDebug("Select code %d.", iResult);

            break;
        }
    }

    // Check if the socket is readable
    if (FD_ISSET(connection->socket, &readSet)) {
        // Call recv() to read data
        iResult = recv(connection->socket, buffer, bufferLength, 0);

        if (iResult == SOCKET_ERROR) {
            lastError = WSAGetLastError();

            if (lastError == WSAEWOULDBLOCK) {
                PrintDebug("No data available to read (would block).");

                return 0; 
            }
            else {
                PrintSocketError("recv failed");

                return iResult;
            }
        }
        else if (iResult == 0) {
            PrintInfo("Connection closed by peer.");
        }
    }

    iResult = recv(connection->socket, buffer, bufferLength, 0);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("recv failed");

        return iResult;
    }

    // Return the total number of bytes received
    return iResult;
}

int SendData(Connection* connection, const char* data, int length) {
    int iResult;
    fd_set writeSet{};
    struct timeval timeout {};
    int lastError;
    int bytesSent = 0;

    // Set a timeout
    timeout.tv_sec = 0;  // Timeout in seconds
    timeout.tv_usec = 0; // Timeout in microseconds

    while (bytesSent < length) {
        while (true) {
            // Initialize the fd_set for the socket
            FD_ZERO(&writeSet);
            FD_SET(connection->socket, &writeSet);

            // Wait for the socket to become writable
            iResult = select(0, NULL, &writeSet, NULL, &timeout);
            if (iResult == SOCKET_ERROR) {
                PrintSocketError("select failed");

                return SOCKET_ERROR;
            }
            else if (iResult == 0) {
                // Timeout occurred
                return iResult;
            }
            else {
                break;
            }
        }

        // Check if the socket is writable
        if (FD_ISSET(connection->socket, &writeSet)) {
            // Attempt to send data
            iResult = send(connection->socket, data + bytesSent, length - bytesSent, 0);
            if (iResult == SOCKET_ERROR) {
                lastError = WSAGetLastError();

                if (lastError == WSAEWOULDBLOCK) {
                    // Would block; try again in the next iteration
                    continue;
                }
                else {
                    PrintSocketError("send failed");

                    return SOCKET_ERROR;
                }
            }

            // Update the number of bytes sent
            bytesSent += iResult;
        }
    }

    // Return the total number of bytes sent
    return bytesSent;
}


void CloseConnection(Connection* connection) {
    if (connection->socket == INVALID_SOCKET) {
        PrintWarning("Trying to close an invalid socket.");
    }

    int iResult = closesocket(connection->socket);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("Closesocket failed");
    }

    InitializeConnection(connection);
}

void ShutdownServer(Connection* serverConnection, Connection clientConnections[], int clientCount) {
    int iResult;

    // Stop accepting new connections on the server socket
    if (serverConnection->socket != INVALID_SOCKET) {
        iResult = shutdown(serverConnection->socket, SD_BOTH);
        if (iResult == SOCKET_ERROR) {
            PrintSocketError("shutdown failed");
        }

        CloseConnection(serverConnection);
    }

    // Close each active client connection
    for (int i = 0; i < clientCount; i++) {
        if (clientConnections[i].socket != INVALID_SOCKET) {
            iResult = shutdown(clientConnections[i].socket, SD_BOTH);
            if (iResult == SOCKET_ERROR) {
                PrintSocketError("shutdown failed");
            }

            CloseConnection(&clientConnections[i]);
        }
    }
}

void ShutdownClient(Connection* clientConnection) {
    int iResult;

    // Check if the client socket is valid
    if (clientConnection->socket != INVALID_SOCKET) {
        // Shutdown the client socket for both send and receive
        iResult = shutdown(clientConnection->socket, SD_BOTH);
        if (iResult == SOCKET_ERROR) {
            PrintSocketError("shutdown failed");
        }

        // Close the socket and clean up the connection
        CloseConnection(clientConnection);
    }
}
