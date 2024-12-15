#include "SenderThread.h"

DWORD WINAPI SenderThread(LPVOID lpParam) {
    PrintDebug("Sender started.");

    // Shared context
    ClientContext* ctx = (ClientContext*)lpParam;

    // Send result
    int sendResult;

    while (true) {
        // Check stop signal
        if (ctx->stopClient) {
            PrintInfo("Stop signal received.");

            break;
        }

        // Send an prepared message with null terminator included
        const char* message = "Hello from Client!";
        PrintDebug("Sending a message to the server: '%s'.", message);
        sendResult = send(ctx->connectSocket, message, (int)strlen(message) + 1, 0);
        if (sendResult == SOCKET_ERROR) {
            PrintCritical("'send' failed with error: %d.", WSAGetLastError());

            return EXIT_FAILURE;
        }

        PrintInfo("Message sent to the server: '%s' with length %d.", message, sendResult);

        Sleep(1000); // Wait for a second
    };

    PrintDebug("Sender stopped.");

    return EXIT_SUCCESS;
}