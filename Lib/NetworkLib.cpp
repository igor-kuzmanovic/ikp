#include "NetworkLib.h"

static void PrintSocketError(const char* message) {
    PrintError("%s with error %d.", message, WSAGetLastError());
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

SOCKET CreateListenSocket(const char* port) {
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

        return INVALID_SOCKET;
    }

    // Create a SOCKET for listening on server
    SOCKET sock = socket(resultingAddress->ai_family, resultingAddress->ai_socktype, resultingAddress->ai_protocol);
    if (sock == INVALID_SOCKET) {
        PrintSocketError("socket failed");
        freeaddrinfo(resultingAddress);

        return INVALID_SOCKET;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    iResult = bind(sock, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("bind failed");
        freeaddrinfo(resultingAddress);
        CloseSocket(sock);

        return INVALID_SOCKET;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set socket in listening mode
    iResult = listen(sock, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("listen failed");
        CloseSocket(sock);

        return INVALID_SOCKET;
    }

    return sock;
}

SOCKET CreateConnectSocket(const char* ipAddress, const char* port) {
    int iResult;

    // Create a socket for connecting to the server
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        PrintSocketError("socket failed");

        return INVALID_SOCKET;
    }

    // Specify server address using sockaddr_in
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress);
    serverAddress.sin_port = htons(atoi(port));

    // Connect to server
    iResult = connect(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("connect failed");
        CloseSocket(sock);

        return INVALID_SOCKET;
    }

    return sock;
}

int SetSocketNonBlocking(SOCKET sock) {
    int iResult;

    // Sets the socket into non-blocking mode
    u_long mode = 1;
    iResult = ioctlsocket(sock, FIONBIO, &mode);
    if (iResult != NO_ERROR) {
        PrintSocketError("ioctlsocket failed");
        CloseSocket(sock);

        return iResult;
    }

    return iResult;
}

SOCKET AcceptSocket(SOCKET serverSock) {
    sockaddr_in address;
    int addressSize = sizeof(struct sockaddr_in);

    SOCKET sock = accept(serverSock, (struct sockaddr*)&address, &addressSize);
    if (sock == INVALID_SOCKET) {
        PrintSocketError("accept failed");

        return INVALID_SOCKET;
    }

    return sock;
}

int ReceiveData(SOCKET sock, char* buffer, int bufferLength) {
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
        FD_SET(sock, &readSet);

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
            break;
        }
    }

    // Check if the socket is readable
    if (FD_ISSET(sock, &readSet)) {
        // Call recv() to read data
        iResult = recv(sock, buffer, bufferLength, 0);

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

    iResult = recv(sock, buffer, bufferLength, 0);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("recv failed");

        return iResult;
    }

    // Return the total number of bytes received
    return iResult;
}

int SendData(SOCKET sock, const char* data, int length) {
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
            FD_SET(sock, &writeSet);

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
        if (FD_ISSET(sock, &writeSet)) {
            // Attempt to send data
            iResult = send(sock, data + bytesSent, length - bytesSent, 0);
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

int CloseSocket(SOCKET sock) {
    if (sock == INVALID_SOCKET) {
        PrintWarning("Trying to close an invalid socket.");
    }

    int iResult = closesocket(sock);
    if (iResult == SOCKET_ERROR) {
        PrintSocketError("closesocket failed");
    }

    return iResult;
}

void ShutdownListenSocket(SOCKET listenSock, SOCKET connectSocks[], int connectSocketCount) {
    int iResult;

    // Stop accepting new connections on the listen socket
    if (listenSock != INVALID_SOCKET) {
        iResult = shutdown(listenSock, SD_BOTH);
        if (iResult == SOCKET_ERROR) {
            PrintSocketError("shutdown failed");
        }

        CloseSocket(listenSock);
    }

    // Close each active connect socket
    for (int i = 0; i < connectSocketCount; i++) {
        if (connectSocks[i] != INVALID_SOCKET) {
            iResult = shutdown(connectSocks[i], SD_BOTH);
            if (iResult == SOCKET_ERROR) {
                PrintSocketError("shutdown failed");
            }

            CloseSocket(connectSocks[i]);
        }
    }
}

void ShutdownConnectSocket(SOCKET connectSock) {
    int iResult;

    // Check if the connect socket is valid
    if (connectSock != INVALID_SOCKET) {
        // Shutdown the connect socket for both send and receive
        iResult = shutdown(connectSock, SD_BOTH);
        if (iResult == SOCKET_ERROR) {
            PrintSocketError("shutdown failed");
        }

        // Close the socket
        CloseSocket(connectSock);
    }
}
