#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#define _WINSOCK_DEPRECATED_NO_WARNINGS	// Disable warnings for networking-related APIs

// System libraries

#include <stdlib.h>
#include <stdio.h>

// Networking libraries

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Need to link with Ws2_32.lib

// User libraries

#include "LoggingLib.h"

// API

// Functions

// Initialize Winsock
int InitializeWindowsSockets();

// Create and bind a TCP socket to a specific port
SOCKET CreateListenSocket(const char* port);

// Create a TCP connect socket and connect to the server
SOCKET CreateConnectSocket(const char* ipAddress, const char* port);

// Accept an incoming client
SOCKET AcceptSocket(SOCKET serverSock);

// Sets the socket into non-blocking mode
int SetSocketNonBlocking(SOCKET sock);

// Calls select until ready for receiving
int SelectForReceive(SOCKET sock);

// Receive data over a TCP socket
int ReceiveData(SOCKET sock, char* buffer);

// Calls select until ready for sending
int SelectForSend(SOCKET sock);

// Send data over a TCP socket
int SendData(SOCKET sock, const char* buffer);
int SendData(SOCKET sock, const char* buffer, int length);

// Close a soket (cleans up Winsock if needed)
int CloseSocket(SOCKET sock);

// Shuts down a server
void ShutdownListenSocket(SOCKET serverSock, SOCKET connectSocks[], int connectSocketCount);

// Shuts down a client
void ShutdownConnectSocket(SOCKET connectSock);
