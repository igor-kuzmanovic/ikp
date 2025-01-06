#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Context
    Context* ctx = (Context*)lpParam;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    // A variable to exponentially increase the sleep time when server is busy
    int serverFullSleepTimeMultiplier = 1;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping receiver.");

            break;
        }

        // Receive data from server
        recvResult = recv(ctx->connectSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Reply received: '%s' with length %d.", receiveBuffer, recvResult);

            // Check if server is shutting down
            if (strstr(receiveBuffer, SERVER_SHUTDOWN_MESSAGE) != NULL) {
                PrintInfo("Server shutdown notification received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(ctx);

                break;
            }
            // Check if server is busy
            else if (strstr(receiveBuffer, SERVER_BUSY_MESSAGE) != NULL) {
                PrintInfo("Server is busy, pausing the sender.");

                PrintDebug("Enabling the pause sender flag.");
                SetPauseSender(ctx, true);

                PrintDebug("Sleeping for %d ms before unpausing the sender.", SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);
                Sleep(SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);

                PrintDebug("Increasing the sleep time multiplier.");
                serverFullSleepTimeMultiplier *= 2;

                PrintDebug("Disabling the pause sender flag.");
                SetPauseSender(ctx, false);

                PrintInfo("Assuming the server is ready to receive new requests, resuming the sender.");

                continue;
            }
            // Check if server is ready
            else if (serverFullSleepTimeMultiplier != 1) {
                PrintDebug("Resetting the sleep time multiplier.");
                serverFullSleepTimeMultiplier = 1;

                continue;
            }
        } else if (recvResult == 0) {
            PrintInfo("Server closed the connection.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }

            continue;
        }
    };

    PrintDebug("Receiver stopped.");

    return TRUE;
}