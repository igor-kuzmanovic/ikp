#include "ClientListenerThread.h"

DWORD WINAPI ClientListenerThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    int iResult;
    int clientId = 1;

    SOCKET clientSocket;

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        clientSocket = SafeAccept(context->clientListenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        } else {
            PrintInfo("New client connected.");

            if (context->clientThreadPool->count < MAX_CLIENTS) {
                iResult = AssignClientDataReceiverThread(context->clientThreadPool, clientSocket, context, clientId++);
                if (iResult == -1) {
                    PrintWarning("Cannot assign client to a client data receiver thread. Rejecting client.");

                    iResult = SafeCloseSocket(&clientSocket);
                    if (iResult != 0) {
                        PrintError("SafeCloseSocket failed with error: %d.", iResult);
                    }
                }
            } else {
                PrintWarning("Maximum client limit reached. Rejecting client.");

                iResult = SafeCloseSocket(&clientSocket);
                if (iResult != 0) {
                    PrintError("SafeCloseSocket failed with error: %d.", iResult);
                }
            }
        }
    }

    return TRUE;
}


