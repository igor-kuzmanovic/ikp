#include "SenderThread.h"

DWORD WINAPI SenderThread(LPVOID lpParam) {
    PrintDebug("Sender started.");

    // Context
    Context* ctx = (Context*)lpParam;

    // Send result
    int sendResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintInfo("Stop signal received, stopping sender.");

            break;
        }

        // Send an prepared message with null terminator included
        const char* message = "Hello from Worker!";
        PrintDebug("Sending a message to the server: '%s'.", message);
        sendResult = send(ctx->connectSocket, message, (int)strlen(message) + 1, 0);
        if (sendResult > 0) {
            PrintInfo("Message sent: '%s' with length %d.", message, sendResult);
        } else if (sendResult == 0) {
            PrintInfo("Server disconnected.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'send' failed with error: %d.", WSAGetLastError());

                break;
            }

            Sleep(BUSY_WAIT_TIME); // Avoid busy waiting

            continue;
        }

        Sleep(1000); // Wait for a second
    };

    PrintDebug("Sender stopped.");

    return TRUE;
}