#include "WorkerListenerThread.h"

DWORD WINAPI WorkerListenerThread(LPVOID lpParam) {
    PrintDebug("Worker listener started.");
    
    Context* ctx = (Context*)lpParam;

    int iResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker handler.");

            break;
        }

        SOCKET workerSocket = accept(ctx->workerListenSocket, NULL, NULL);
        if (workerSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }

            continue;
        } else {
            PrintInfo("New worker connected.");

            // TODO Remove workerCount from the context and use the workerList->count instead
            if (ctx->workerCount < MAX_WORKERS) {
                PrintDebug("Adding the worker to the worker list.");
                iResult = AddWorker(ctx->workerList, workerSocket);
                if (iResult == -1) {
                    PrintWarning("Cannot add worker to the worker list. Rejecting worker.");
                    
                    PrintDebug("Closing the worker socket.");
                    closesocket(workerSocket);
                } else {
                    ctx->workerCount++;
                }
            } else {
                PrintWarning("Maximum worker limit reached. Rejecting worker.");

                PrintDebug("Closing the worker socket.");
                closesocket(workerSocket);
            }
        }
    }

    PrintDebug("Worker listener stopped.");

    return TRUE;
}
