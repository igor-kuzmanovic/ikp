#include "LoadBalancer.h"

int main(void) {
    PrintDebug("Load Balancer started.");

    int iResult;

    // Initialize Winsock
    PrintDebug("Initializing Winsock.");
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;

    // Socket used for communication with client
    SOCKET acceptedSockets[4] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    PrintDebug("Resolving the server address and port.");
    iResult = getaddrinfo(NULL, SERVER_PORT, &hints, &resultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for client connections
    PrintDebug("Creating the listen socket.");
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Free address information
        freeaddrinfo(resultingAddress);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    PrintDebug("Binding the listen socket.");
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
        closesocket(listenSocket);

        // Free address information
        freeaddrinfo(resultingAddress);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    PrintDebug("Setting the listen socket in listening mode.");
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
        closesocket(listenSocket);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    PrintInfo("Server initialized, waiting for clients.");

    // An index to keep track of the accepted sockets
    int i = 0;

    // A variable to store the result of recv
    int recvResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    do {
        // Wait for clients and accept client connections
        PrintDebug("Waiting for a client to connect.");
        acceptedSockets[i] = accept(listenSocket, NULL, NULL);
        if (acceptedSockets[i] == INVALID_SOCKET) {
            PrintError("'accept' failed with error: %d.", WSAGetLastError());

            continue;
        }

        do {
            // Receive data until the client shuts down the connection
            PrintDebug("Waiting for a message from the client %d.", i);
            recvResult = recv(acceptedSockets[i], receiveBuffer, BUFFER_SIZE, 0);
            if (recvResult > 0) {
                PrintInfo("Message received from a client %d: '%s' with length %d.", i, receiveBuffer, recvResult);
            } else if (recvResult == 0) {
                // Connection was closed gracefully
                PrintInfo("Connection with client closed.");

                // Close the accepted socket
                PrintDebug("Closing the accepted socket %d.", i);
                iResult = closesocket(acceptedSockets[i]);
                if (iResult == SOCKET_ERROR) {
                    PrintError("'closesocket' failed with error: %d.", WSAGetLastError());

                    continue;
                }
            } else {
                // There was an error during recv
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                // Close the accepted socket
                PrintDebug("Closing the accepted socket %d.", i);
                iResult = closesocket(acceptedSockets[i]);
                if (iResult == SOCKET_ERROR) {
                    PrintError("'closesocket' failed with error: %d.", WSAGetLastError());

                    continue;
                }
            }

            // Message to reply with
            const char* messageToReply = "this is a test reply";

            // Send an message back with null terminator included
            PrintDebug("Replying to the client %d with: '%s'.", i, messageToReply);
            sendResult = send(acceptedSockets[i], messageToReply, (int)strlen(messageToReply) + 1, 0);
            if (sendResult == SOCKET_ERROR) {
                PrintError("'send' failed with error: %d.", WSAGetLastError());

                continue;
            }
        } while (recvResult > 0);

        // Increment the index
        i = (i + 1) % 4;
    } while (true);

    // Shutdown the connection since we're done
    for (int i = 0; i < 4; i++) {
        PrintDebug("Shutting down the accepted socket %d.", i);
        iResult = shutdown(acceptedSockets[i], SD_BOTH);
        if (iResult == SOCKET_ERROR) {
            PrintCritical("'shutdown' failed with error: %d.", WSAGetLastError());

            // Close the accepted socket
            closesocket(acceptedSockets[i]);

            // Close the listen socket
            closesocket(listenSocket);

            // Cleanup Winsock
            WSACleanup();

            return EXIT_FAILURE;
        }
    }

    // Close the accepted sockets
    for (int i = 0; i < 4; i++) {
        PrintDebug("Closing the accepted socket %d.", i);
        iResult = closesocket(acceptedSockets[i]);
        if (iResult == SOCKET_ERROR) {
            PrintCritical("'closesocket' failed with error: %d.", WSAGetLastError());

            // Close the listen socket
            closesocket(listenSocket);

            // Cleanup Winsock
            WSACleanup();

            return EXIT_FAILURE;
        }
    }

    // Close the listen socket
    PrintDebug("Closing the listen socket.");
    iResult = closesocket(listenSocket);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Cleanup Winsock
    WSACleanup();

    PrintDebug("Load Balancer stopped.");

    PrintInfo("Press any key to exit.");

    int _ = getchar(); // Wait for key press

    return EXIT_SUCCESS;
}
