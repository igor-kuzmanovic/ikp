#include "LoadBalancer.h"

int main(void) {
    // Variable used to store function return value
    int iResult;

    // Socket used for listening for new clients
    Connection serverConnection;
    InitializeConnection(&serverConnection);

    // Socket used for communication with client
    Connection clientConnection;
    InitializeConnection(&clientConnection);
    Connection clientConnections[] = { clientConnection };

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE];

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP listening socket
    iResult = CreateServerSocket(&serverConnection, SERVER_PORT);
    if (iResult != 0) {
        CloseConnection(&serverConnection);
        WSACleanup();

        return 1;
    }

    PrintInfo("Server initialized, waiting for clients.");

    do {
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.
        iResult = AcceptConnection(&serverConnection, &clientConnection);
        if (iResult != 0) {
            CloseConnection(&serverConnection);
            WSACleanup();

            return 1;
        }

        do {
            // Receive data until the client shuts down the connection
            iResult = ReceiveData(&clientConnection, receiveBuffer, BUFFER_SIZE + 1);
            if (iResult > 0) {
                PrintInfo("Message received from client: %s.", receiveBuffer);
            }
            else if (iResult == 0) {
                PrintInfo("Connection with client closed.");

                CloseConnection(&clientConnection);
            }
            else {
                PrintDebug("Receive code %d.", iResult);

                CloseConnection(&clientConnection);
            }
        } while (iResult > 0);
    } while (1);

    // Shutdown the connection and server since we're done
    ShutdownServer(&serverConnection, clientConnections, 1);

    // Cleanup Winsock
    WSACleanup();

    int _ = getchar(); // Wait for key press

    return 0;
}
