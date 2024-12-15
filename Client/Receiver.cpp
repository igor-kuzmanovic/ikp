#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Socket used for communicating with the server
    SOCKET connectSocket = *(SOCKET*)lpParam;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    do {
        // Receive data until the client shuts down the connection
        PrintDebug("Waiting for a message from the server.");
        recvResult = recv(connectSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received from the server: '%s' with length %d.", receiveBuffer, recvResult);
        } else if (recvResult == 0) {
            // Connection was closed gracefully
            PrintInfo("Connection with server closed.");
        } else {
            // There was an error during recv
            PrintError("'recv' failed with error: %d.", WSAGetLastError());

            continue;
        }
    } while (recvResult > 0);

    PrintDebug("Receiver stopped.");

    return EXIT_SUCCESS;
}