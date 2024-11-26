#include "LoadBalancer.h"

int main(void) {
    // Variable used to store function return value
    int iResult;

    // Socket used for listening for new clients
    SOCKET serverSock;

    // Socket used for communication with client
    SOCKET clientSock;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE];

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP listening socket
    serverSock = CreateListenSocket(SERVER_PORT);
    if (iResult == INVALID_SOCKET) {
        CloseSocket(serverSock);
        WSACleanup();

        return 1;
    }

    PrintInfo("Server initialized, waiting for clients.");

    do {
        // Wait for clients and accept client connections.
        clientSock = AcceptSocket(serverSock);
        if (clientSock == INVALID_SOCKET) {
            CloseSocket(clientSock);
            
            continue;
        }

        do {
            // Receive data until the client shuts down the socket
            iResult = ReceiveData(clientSock, receiveBuffer);
            if (iResult > 0) {
                PrintInfo("Message received from client: %s.", receiveBuffer);
            }
            else if (iResult == 0) {
                PrintInfo("Connection with client closed.");

                CloseSocket(clientSock);
            }
            else {
                PrintDebug("Receive code %d.", iResult);

                CloseSocket(clientSock);
            }
        } while (iResult > 0);
    } while (1);

    // Shutdown the socket and server since we're done
    SOCKET clientSocks[] = { clientSock };
    ShutdownListenSocket(serverSock, clientSocks, 1);

    // Cleanup Winsock
    WSACleanup();

    int _ = getchar(); // Wait for key press

    return 0;
}
