#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    // Socket used for communicating with the server
    SOCKET sock = *(SOCKET *)lpParam;

    // Variable used to store function return value
    int iResult = 0;

    // Expected result
    const char* expectedResult = "success";

    // Buffer used for receiving messages
    char* receiveBuffer = (char*)malloc(BUFFER_SIZE + sizeof(char));

    int messageCount = 5;
    for (int i = 0; i < messageCount; i++) {
        // Receive data until the server shuts down the socket
        iResult = ReceiveData(sock, receiveBuffer, BUFFER_SIZE);
        if (iResult > 0) {
            PrintInfo("Message received from server: %s.", receiveBuffer);
        }
        else if (iResult == 0) {
            // Connection was closed gracefully
            PrintInfo("Connection with server closed.");

            break;
        }
        else {
            break;
        }
    };

    // Free the memory used by the receive buffer
    free(receiveBuffer);

    return 0;
}