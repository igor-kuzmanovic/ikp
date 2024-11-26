#include "Sender.h"

DWORD WINAPI StartSender(LPVOID lpParam) {
    // Socket used for communicating with server
    SOCKET sock = *(SOCKET*)lpParam;

    // Variable used to store function return value
    int iResult = 0;

    // Buffer used for sending messages
    char* sendBuffer = (char*)malloc(BUFFER_SIZE);
    if (sendBuffer != NULL) {
        // Fill the buffer with 0s
        memset(sendBuffer, 0, BUFFER_SIZE);  
    } else {
        PrintError("Can't allocate memory for send buffer.");

        return 1;
    }

    // Message to send
    const char* messageTemplate = "foo %d\0";
    char messageToSend[BUFFER_SIZE];

    int messageCount = 5;
    for (int i = 0; i < messageCount; i++) {
        // Periodically sends a request to the server
        Sleep(1000);

        // Create the message from template
        snprintf(messageToSend, sizeof(messageToSend), messageTemplate, i + 1);
        
        // Copy the formatted message into sendBuffer to be sent
        snprintf(sendBuffer, BUFFER_SIZE, "%s", messageToSend);
        
        // Ensure null-termination
        sendBuffer[BUFFER_SIZE - 1] = '\0'; 

        // Send the message
        iResult = send(sock, sendBuffer, (int)strlen(sendBuffer), 0);
        if (iResult > 0) {
            PrintInfo("Bytes sent: %ld, message: %s.", iResult, messageToSend);
        } else if (iResult == 0 || WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED) {
            PrintInfo("Connection with server closed.");

            break;
        } else {
            PrintError("send failed with error %d.", WSAGetLastError());

            break;
        }
    };

    // Free the memory used by the send buffer
    free(sendBuffer);

    return 0;
}