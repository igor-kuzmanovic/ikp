#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Shared context
    ClientContext* ctx = (ClientContext*)lpParam;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    while (true) {
        // Check stop signal
        if (ctx->stopClient) {
            PrintInfo("Stop signal received.");

            break;
        }

        // Receive data from server
        int recvResult = recv(ctx->connectSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Reply received: '%s' with length %d.", receiveBuffer, recvResult);

            // Check if server is shutting down
            if (strstr(receiveBuffer, "Server is shutting down.") != NULL) {
                PrintWarning("Server shutdown notification received. Stopping client...");

                // Acquire lock to safely update the stop flag
                EnterCriticalSection(&ctx->lock);
                ctx->stopClient = true;
                LeaveCriticalSection(&ctx->lock);

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
        }
    };

    PrintDebug("Receiver stopped.");

    return EXIT_SUCCESS;
}