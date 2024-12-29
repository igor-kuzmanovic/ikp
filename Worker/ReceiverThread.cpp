#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Context
    Context* ctx = (Context*)lpParam;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping receiver.");

            break;
        }

        // Receive data from server
        recvResult = recv(ctx->connectSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Client request received: '%s' with length %d.", receiveBuffer, recvResult);

            // Check if server is shutting down
            if (strstr(receiveBuffer, "Server is shutting down.") != NULL) {
                PrintInfo("Server shutdown notification received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(ctx);

                break;
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

            Sleep(INPUT_WAIT_TIME);

            continue;
        }
    };

    PrintDebug("Receiver stopped.");

    return TRUE;
}