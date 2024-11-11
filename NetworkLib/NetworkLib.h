#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define _WINSOCK_DEPRECATED_NO_WARNINGS	// Disable warnings for networking-related APIs

// System libraries

#include <stdio.h>

// Networking libraries

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")      // Need to link with Ws2_32.lib

// API

// Structures

// Define a structure for easier management of sockets and addresses
typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
} Connection;

// Functions

// Initialize a Connection structure
void InitializeConnection(Connection* connection);

// Initialize Winsock
int InitializeWindowsSockets();

// Create and bind a TCP socket to a specific port
int CreateServerSocket(Connection* connection, const char* port);

// Create a TCP client socket and connect to the server
int CreateClientSocket(Connection* connection, const char* ipAddress, const unsigned short port);

// Accept an incoming client connection
int AcceptConnection(Connection* serverConnection, Connection* clientConnection);

// Receive data over a TCP connection
int ReceiveData(Connection* connection, char* buffer, int bufferLength);

// Send data over a TCP connection
int SendData(Connection* connection, const char* data, int length);

// Close a connection (cleans up Winsock if needed)
void CloseConnection(Connection* conn);

// Shuts down a server
void ShutdownServer(Connection* serverConnection, Connection clientConnections[], int clientCount);

// Helper function to print socket errors
void PrintSocketError(const char* message);
