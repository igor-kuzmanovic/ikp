#include "NetworkLib.h"

#define NETWORKLIB_NAMESPACE "NetLib"
#define PrintDebug(format, ...) PrintDebugFunc(NETWORKLIB_NAMESPACE, format, ##__VA_ARGS__)
#define PrintInfo(format, ...) PrintInfoFunc(NETWORKLIB_NAMESPACE, format, ##__VA_ARGS__)
#define PrintWarning(format, ...) PrintWarningFunc(NETWORKLIB_NAMESPACE, format, ##__VA_ARGS__)
#define PrintError(format, ...) PrintErrorFunc(NETWORKLIB_NAMESPACE, format, ##__VA_ARGS__)
#define PrintCritical(format, ...) PrintCriticalFunc(NETWORKLIB_NAMESPACE, format, ##__VA_ARGS__)

int InitializeWindowsSockets() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());
        return iResult;
    }
    return 0;
}

int SafeSend(SOCKET socket, const char* buffer, int length) {
    if (socket == INVALID_SOCKET || buffer == NULL || length <= 0) {
        PrintError("Invalid parameters for SafeSend");
        return -1;
    }

    int bytesSent = send(socket, buffer, length, 0);
    
    if (bytesSent == SOCKET_ERROR) {
        int error = WSAGetLastError();
        
        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
        else if (error == WSANOTINITIALISED) {
            return -2;
        }
        else if (error == WSAECONNRESET || error == WSAECONNABORTED) {
            return -2;
        }
        else if (error == WSAENETUNREACH || error == WSAEHOSTUNREACH) {
            PrintError("Network/host unreachable during send (error: %d)", error);
            return -3;
        }
        else {
            PrintError("send() failed with error: %d", error);
            return -1;
        }
    }
    
    if (bytesSent == 0) {
        return -2;
    }
    
    return bytesSent;
}

int SafeReceive(SOCKET socket, char* buffer, int length) {
    if (socket == INVALID_SOCKET || buffer == NULL || length <= 0) {
        PrintError("Invalid parameters for SafeReceive");
        return -1;
    }

    int bytesReceived = recv(socket, buffer, length, 0);
    
    if (bytesReceived == SOCKET_ERROR) {
        int error = WSAGetLastError();
        
        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
        else if (error == WSANOTINITIALISED) {
            return -2;
        }
        else if (error == WSAECONNRESET || error == WSAECONNABORTED) {
            return -2;
        }
        else if (error == WSAENETUNREACH || error == WSAEHOSTUNREACH) {
            PrintError("Network/host unreachable during receive (error: %d)", error);
            return -3;
        }
        else {
            PrintError("recv() failed with error: %d", error);
            return -1;
        }
    }
    
    if (bytesReceived == 0) {
        return -2;
    }
    
    return bytesReceived;
}

int SafeCloseSocket(SOCKET* socket) {
    if (!socket || *socket == INVALID_SOCKET) {
        return 0;
    }
    
    if (shutdown(*socket, SD_BOTH) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAENOTCONN && error != WSAENOTSOCK && error != WSANOTINITIALISED) { 
            PrintWarning("Socket shutdown failed: %d", error);
        }
    }
    
    int result = closesocket(*socket);
    SOCKET oldSocket = *socket;
    *socket = INVALID_SOCKET;
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAENOTSOCK && error != WSANOTINITIALISED) {
            PrintError("Socket close failed with error: %d", error);
            return -1;
        }
    }
    
    return 0;
}

int SendBuffer(SOCKET socket, const char* buffer, int length) {
    if (socket == INVALID_SOCKET || buffer == NULL || length <= 0) {
        PrintError("Invalid parameters for SendBuffer");
        return -1;
    }

    int totalSent = 0;
    int retries = 0;
    const int MAX_RETRIES = 5000;
    
    while (totalSent < length && retries < MAX_RETRIES) {
        int bytesSent = SafeSend(socket, buffer + totalSent, length - totalSent);
        
        if (bytesSent > 0) {
            totalSent += bytesSent;
            retries = 0;
        }
        else if (bytesSent == 0) {
            retries++;
            Sleep(NETWORK_POLLING_DELAY);
        }
        else if (bytesSent == -2 || bytesSent == -3) {
            return bytesSent;
        }
        else {
            PrintError("SendBuffer: Send operation failed with error code %d", bytesSent);
            return -1;
        }
    }
    
    if (totalSent < length) {
        PrintError("SendBuffer: Failed to send complete message after %d retries (%d of %d bytes)", retries, totalSent, length);
        return -1;
    }

    return 0;
}

int ReceiveBuffer(SOCKET socket, char* buffer, int length) {
    if (socket == INVALID_SOCKET || buffer == NULL || length <= 0) {
        PrintError("Invalid parameters for ReceiveBuffer");
        return -1;
    }

    int totalReceived = 0;
    int retries = 0;
    const int MAX_RETRIES = 5000;
    
    while (totalReceived < length && retries < MAX_RETRIES) {
        int bytesReceived = SafeReceive(socket, buffer + totalReceived, length - totalReceived);
        
        if (bytesReceived > 0) {
            totalReceived += bytesReceived;
            retries = 0;
        }
        else if (bytesReceived == 0) {
            retries++;
            Sleep(NETWORK_POLLING_DELAY);
        }
        else if (bytesReceived == -2 || bytesReceived == -3) {
            return bytesReceived;
        }
        else {
            PrintError("ReceiveBuffer: Receive operation failed with error code %d", bytesReceived);
            return -1;
        }
    }
    
    if (totalReceived < length) {
        PrintError("ReceiveBuffer: Failed to receive complete message after %d retries (%d of %d bytes)", retries, totalReceived, length);
        return -1;
    }
    
    return 0;
}

int SetSocketNonBlocking(SOCKET socket) {
    if (socket == INVALID_SOCKET) {
        PrintError("Invalid socket for SetSocketNonBlocking");
        return -1;
    }

    u_long socketMode = 1;
    int iResult = ioctlsocket(socket, FIONBIO, &socketMode);
    if (iResult == SOCKET_ERROR) {
        PrintError("ioctlsocket failed with error: %d", WSAGetLastError());
        return -1;
    }

    return 0;
}


int SafeConnect(SOCKET socket, const struct sockaddr* addr, int addrlen, int timeoutSeconds) {
    if (socket == INVALID_SOCKET || addr == NULL || addrlen <= 0) {
        PrintError("Invalid parameters for SafeConnect");
        return -1;
    }
    
    int result = connect(socket, addr, addrlen);
    if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAEWOULDBLOCK) {
            fd_set writeSet;
            struct timeval timeout;
            timeout.tv_sec = timeoutSeconds;
            timeout.tv_usec = 0;
            
            FD_ZERO(&writeSet);
            FD_SET(socket, &writeSet);
            
            result = select(0, NULL, &writeSet, NULL, &timeout);
            if (result == SOCKET_ERROR) {
                int selectError = WSAGetLastError();
                if (selectError != WSAENOTSOCK && selectError != WSANOTINITIALISED) {
                    PrintError("select() failed with error: %d", selectError);
                }
                return -1;
            } else if (result == 0) {
                PrintError("Connection attempt timed out");
                return -2;  
            } else {
                int sockError = 0;
                int sockErrorLen = sizeof(sockError);
                if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&sockError, &sockErrorLen) == SOCKET_ERROR) {
                    int getsockoptError = WSAGetLastError();
                    if (getsockoptError != WSAENOTSOCK && getsockoptError != WSANOTINITIALISED) {
                        PrintError("getsockopt() failed with error: %d", getsockoptError);
                    }
                    return -1;
                }
                
                if (sockError != 0) {
                    PrintError("Connection failed with error: %d", sockError);
                    return -1;
                }
            }
        } else {
            PrintError("connect() failed with error: %d", errorCode);
            return -1;
        }
    }
    
    return 0;
}

SOCKET SafeAccept(SOCKET listenSocket, struct sockaddr* addr, int* addrlen) {
    if (listenSocket == INVALID_SOCKET) {
        PrintError("Invalid listen socket for SafeAccept");
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = accept(listenSocket, addr, addrlen);
    if (clientSocket == INVALID_SOCKET) {
        int errorCode = WSAGetLastError();
        if (errorCode == WSAEWOULDBLOCK) {
            return INVALID_SOCKET;
        } else {
            PrintError("accept() failed with error: %d", errorCode);
            return INVALID_SOCKET;
        }
    }

    SetSocketNonBlocking(clientSocket);
    return clientSocket;
}

int IsSocketReadyToRead(SOCKET socket, int timeoutMs) {
    if (socket == INVALID_SOCKET) {
        PrintError("Invalid socket for IsSocketReadyToRead");
        return -1;
    }

    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);
    
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(0, &readfds, NULL, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        PrintWarning("select() failed with error: %d", error);
        return -1;
    }
    
    return (result > 0 && FD_ISSET(socket, &readfds)) ? 1 : 0;
}

int IsSocketReadyToWrite(SOCKET socket, int timeoutMs) {
    if (socket == INVALID_SOCKET) {
        PrintError("Invalid socket for IsSocketReadyToWrite");
        return -1;
    }

    fd_set writefds;
    struct timeval timeout;
    
    FD_ZERO(&writefds);
    FD_SET(socket, &writefds);
    
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(0, NULL, &writefds, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        PrintWarning("select() failed with error: %d", error);
        return -1;
    }
    
    return (result > 0 && FD_ISSET(socket, &writefds)) ? 1 : 0;
}

int WaitForMultipleSockets(SOCKET* sockets, int socketCount, int timeoutMs) {
    if (sockets == NULL || socketCount <= 0 || socketCount > FD_SETSIZE) {
        PrintError("Invalid parameters for WaitForMultipleSockets");
        return -1;
    }

    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    for (int i = 0; i < socketCount; i++) {
        if (sockets[i] != INVALID_SOCKET) {
            FD_SET(sockets[i], &readfds);
        }
    }
    
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(0, &readfds, NULL, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        PrintWarning("select() failed with error: %d", error);
        return -1;
    }
    
    if (result == 0) {
        return 0;
    }
    
    for (int i = 0; i < socketCount; i++) {
        if (sockets[i] != INVALID_SOCKET && FD_ISSET(sockets[i], &readfds)) {
            return i + 1; 
        }
    }
    
    return 0;
}


