#include "SenderThread.h"

DWORD WINAPI SenderThread(LPVOID lpParam) {
    PrintDebug("Sender started.");

    // Context
    Context* ctx = (Context*)lpParam;

    // Buffer used for storing outgoing data
    char sendBuffer[BUFFER_SIZE]{};

    // Send result
    int sendResult;

    // A variable to exponentially increase the sleep time when server is busy
    int serverFullSleepTimeMultiplier = 1;

    // Message counter
    int messageCounter = 0;

    if (MESSAGE_COUNT != INFINITE) {
        PrintInfo("Sending %d messages to the server.", MESSAGE_COUNT);
    } else {
        PrintInfo("Sending messages to the server until the stop signal is received.");
    }

    while (MESSAGE_COUNT == INFINITE || messageCounter < MESSAGE_COUNT) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping sender.");

            break;
        }

        // Check if the sender should pause
        if (GetPauseSender(ctx)) {
            // TODO Probably don't need to sleep here since the receiver will sleep
            PrintDebug("Sleeping for %d ms before sending new requests.", SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);
            Sleep(SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);

            PrintDebug("Increasing the sleep time multiplier.");
            serverFullSleepTimeMultiplier *= 2;

            continue;
        } else if (serverFullSleepTimeMultiplier != 1) {
            PrintDebug("Resetting the sleep time multiplier.");
            serverFullSleepTimeMultiplier = 1;
        }

        // Send an prepared message with null terminator included
        GenerateClientMessage(ctx->connectSocket, sendBuffer, BUFFER_SIZE, ++messageCounter);
        PrintDebug("Sending a message to the server: '%s'.", sendBuffer);
        sendResult = send(ctx->connectSocket, sendBuffer, (int)strlen(sendBuffer) + 1, 0);
        if (sendResult > 0) {
            PrintInfo("Message sent: '%s' with length %d.", sendBuffer, sendResult);
        } else if (sendResult == 0) {
            PrintInfo("Server disconnected.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("[SenderThread] 'send' failed with error: %d.", WSAGetLastError());

                break;
            }

            continue;
        }

        Sleep(MESSAGE_SEND_WAIT_TIME); // Wait for a second
    };

    PrintDebug("Sender stopped.");

    return TRUE;
}
