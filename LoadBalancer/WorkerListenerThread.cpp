#include "WorkerListenerThread.h"

DWORD WINAPI WorkerListenerThread(LPVOID lpParam) {
    PrintDebug("Worker listener started.");
    
    Context* context = (Context*)lpParam;

    int iResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker handler.");

            break;
        }

        SOCKET workerSocket = accept(context->workerListenSocket, NULL, NULL);
        if (workerSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }

            continue;
        } else {
            PrintInfo("New worker connected.");

            if (context->workerList->count < MAX_WORKERS) {
                PrintDebug("Adding the worker to the worker list.");
                iResult = AddWorker(context->workerList, workerSocket);
                if (iResult == -1) {
                    PrintWarning("Cannot add worker to the worker list. Rejecting worker.");
                    
                    PrintDebug("Closing the worker socket.");
                    iResult = closesocket(workerSocket);
                    if (iResult == SOCKET_ERROR) {
                        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
                    }
                }
            } else {
                PrintWarning("Maximum worker limit reached. Rejecting worker.");

                PrintDebug("Closing the worker socket.");
                iResult = closesocket(workerSocket);
                if (iResult == SOCKET_ERROR) {
                    PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
                }
            }
        }
    }

    PrintDebug("Worker listener stopped.");

    return TRUE;
}
