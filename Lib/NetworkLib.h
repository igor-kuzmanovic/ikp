#pragma once

#include "SharedConfig.h"
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include "LoggingLib.h"

#define NETWORK_ERROR_GENERAL        -1
#define NETWORK_ERROR_CONN_CLOSED    -2
#define NETWORK_ERROR_UNREACHABLE    -3

int InitializeWindowsSockets();
int SafeSend(SOCKET socket, const char* buffer, int length);
int SafeReceive(SOCKET socket, char* buffer, int length);
int SafeCloseSocket(SOCKET* socket);
int SendBuffer(SOCKET socket, const char* buffer, int length);
int ReceiveBuffer(SOCKET socket, char* buffer, int length);
int SetSocketNonBlocking(SOCKET socket);
int SafeConnect(SOCKET socket, const struct sockaddr* addr, int addrlen, int timeoutSeconds);
SOCKET SafeAccept(SOCKET listenSocket, struct sockaddr* addr, int* addrlen);

int IsSocketReadyToRead(SOCKET socket, int timeoutMs);
int IsSocketReadyToWrite(SOCKET socket, int timeoutMs);
int WaitForMultipleSockets(SOCKET* sockets, int socketCount, int timeoutMs);
