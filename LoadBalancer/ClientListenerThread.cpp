#include "ClientListenerThread.h"

DWORD WINAPI ClientListenerThread(LPVOID lpParam) {
    PrintDebug("Client listener started.");

    Context* ctx = (Context*)lpParam;

    int iResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping client handler.");

            break;
        }

        SOCKET clientSocket = accept(ctx->clientListenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }

            continue;
        } else {
            PrintInfo("New client connected.");

            // TODO Remove clientCount from the context and use the clientThreadPool->count instead
            if (ctx->clientCount < MAX_CLIENTS) {
                PrintDebug("Assigning the client to a client data receiver thread.");
                iResult = AssignClientDataReceiverThread(ctx->clientThreadPool, clientSocket, ctx);
                if (iResult == -1) {
                    PrintWarning("Cannot assign client to a client data receiver thread. Rejecting client.");

                    PrintDebug("Closing the client socket.");
                    closesocket(clientSocket);
                } else {
                    ctx->clientCount++;
                }
            } else {
                PrintWarning("Maximum client limit reached. Rejecting client.");

                PrintDebug("Closing the client socket.");
                closesocket(clientSocket);
            }
        }
    }

    PrintDebug("Client listener stopped.");

    return TRUE;
}
