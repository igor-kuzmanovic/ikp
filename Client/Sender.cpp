#include "Sender.h"

DWORD WINAPI StartSender(LPVOID lpParam) {
    // Socket used for communicating with server
    Connection connection = *(Connection*)lpParam;

    // Variable used to store function return value
    int iResult = 0;

    // Message to send
    const char* messageTemplate = "foo %d";
    char messageToSend[BUFFER_SIZE];

    int messageCount = 5;
    for (int i = 0; i < messageCount; i++) {
        // Create the message from template
        snprintf(messageToSend, sizeof(messageToSend), messageTemplate, i);

        // Periodically sends a request to the server
        Sleep(1000);

        // Send an prepared message with null terminator included
        iResult = SendData(&connection, messageToSend, (int)strlen(messageToSend) + 1);
        if (iResult == SOCKET_ERROR) {
            CloseConnection(&connection);
            WSACleanup();

            return 1;
        }

        PrintInfo("Bytes sent: %ld, message: %s.", iResult, messageToSend);
    };

    return 0;
}