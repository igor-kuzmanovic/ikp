#include "Sender.h"

DWORD WINAPI StartSender(LPVOID lpParam) {
    PrintDebug("Sender started.");

    // Socket used for communicating with the server
    SOCKET connectSocket = *(SOCKET*)lpParam;

    // Send result
    int sendResult;

    // Message to send
    const char* messageToSend = "this is a test";

    do {
        // Send an prepared message with null terminator included
        PrintDebug("Sending a message to the server: '%s'.", messageToSend);
        sendResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);
        if (sendResult == SOCKET_ERROR) {
            PrintCritical("'send' failed with error: %d.", WSAGetLastError());

            return EXIT_FAILURE;
        }

        PrintInfo("Message sent to the server: '%s' with length %d.", messageToSend, sendResult);

        int key = getchar(); // Wait for key press
        if (key == 'q') {
            break;
        }
    } while (sendResult > 0);

    PrintDebug("Sender stopped.");

    return EXIT_SUCCESS;
}