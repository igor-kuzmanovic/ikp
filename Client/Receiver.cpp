#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    // Socket used for communicating with the server
    SOCKET sock = *(SOCKET*)lpParam;

    // Variable used to store function return value
    int iResult = 0;

    // Buffer used for receiving messages
    char* receiveBuffer = (char*)malloc(BUFFER_SIZE);

    int messageCount = 5;
    for (int i = 0; i < messageCount; i++) {
        // Receive data
        iResult = ReceiveData(sock, receiveBuffer);
        if (iResult > 0) {
            PrintInfo("Bytes received: %ld, message: %s.", iResult, receiveBuffer);
        } else if (iResult == 0 || WSAGetLastError() == WSAECONNRESET) {
            PrintInfo("Connection with server closed.");

            break;
        } else {
            PrintError("recv failed with error %d.", WSAGetLastError());

            break;
        }
    };

    // Free the memory used by the receive buffer
    free(receiveBuffer);

    return 0;
}