#include "Client.h"

int main()
{
    // Variable used to store function return value
    int iResult;
    // Socket used for communicating with server
    Connection connection;
    InitializeConnection(&connection);
    // Message to send
    const char* messageToSend = "this is a test";

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP connecting socket
    iResult = CreateClientSocket(&connection, DEFAULT_ADDRESS, DEFAULT_PORT);
    if (iResult != 0) {
        CloseConnection(&connection);
        WSACleanup();

        return 1;
    }

    // Send an prepared message with null terminator included
	iResult = SendData(&connection, messageToSend, (int)strlen(messageToSend) + 1);
    if (iResult == SOCKET_ERROR)
    {
        CloseConnection(&connection);
        WSACleanup();

        return 1;
    }
        
	printf("Bytes Sent: %ld\n", iResult);

    // Close the connection
    CloseConnection(&connection);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
