#include "Sender.h"

DWORD WINAPI StartSender(LPVOID lpParam) {
    // Variable used to store function return value
    int iResult = 0;

    // Message to send
    const char* messageToSend = "this is a test";

    // Socket used for communicating with server
    Connection connection = *(Connection*)lpParam;

    for (int i = 0; i < 5; i++) {
        // Send an prepared message with null terminator included
        iResult = SendData(&connection, messageToSend, (int)strlen(messageToSend) + 1);
        if (iResult == SOCKET_ERROR) {
            CloseConnection(&connection);
            WSACleanup();

            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);

        // Periodically sends a request to the server
        Sleep(5000);
    }

    return 0;
}