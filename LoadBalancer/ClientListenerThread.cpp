#include "ClientListenerThread.h"

DWORD WINAPI ClientListenerThread(LPVOID lpParam) {
    PrintDebug("Client listener started.");

    Context* context = (Context*)lpParam;

    int iResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping client handler.");

            break;
        }

        SOCKET clientSocket = accept(context->clientListenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }

            continue;
        } else {
            PrintInfo("New client connected.");

            if (context->clientThreadPool->count < MAX_CLIENTS) {
                PrintDebug("Assigning the client to a client data receiver thread.");
                iResult = AssignClientDataReceiverThread(context->clientThreadPool, clientSocket, context);
                if (iResult == -1) {
                    PrintWarning("Cannot assign client to a client data receiver thread. Rejecting client.");

                    PrintDebug("Closing the client socket.");
                    iResult = closesocket(clientSocket);
                    if (iResult == SOCKET_ERROR) {
                        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
                    }
                }
            } else {
                PrintWarning("Maximum client limit reached. Rejecting client.");

                PrintDebug("Closing the client socket.");
                iResult = closesocket(clientSocket);
                if (iResult == SOCKET_ERROR) {
                    PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
                }
            }
        }
    }

    PrintDebug("Client listener stopped.");

    return TRUE;
}
