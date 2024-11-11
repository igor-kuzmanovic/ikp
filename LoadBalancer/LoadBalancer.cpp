#include "LoadBalancer.h"

int main(void)
{
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
    char recieveBuffer[DEFAULT_BUFLEN];

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP listening socket
    iResult = CreateServerSocket(&serverConnection, DEFAULT_PORT_STR);
    if (iResult != 0) {
        CloseConnection(&serverConnection);
        WSACleanup();

        return 1;
    }

    printf("Server initialized, waiting for clients.\n");

    do
    {
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.
        iResult = AcceptConnection(&serverConnection, &clientConnection);
        if (iResult != 0)
        {
            CloseConnection(&serverConnection);
            WSACleanup();

            return 1;
        }

        do
        {
            // Receive data until the client shuts down the connection
            iResult = ReceiveData(&clientConnection, recieveBuffer, DEFAULT_BUFLEN);
            if (iResult > 0)
            {
                printf("Message received from client: %s.\n", recieveBuffer);
            }
            else if (iResult == 0)
            {
                // Connection was closed gracefully
                printf("Connection with client closed.\n");

                CloseConnection(&clientConnection);
            }
            else
            {
                CloseConnection(&clientConnection);
            }
        } while (iResult > 0);

        // Here is where server shutdown logic could be placed

    } while (1);

    // Shutdown the connection and server since we're done
    ShutdownServer(&serverConnection, clientConnections, 1);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
