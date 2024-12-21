#include "WorkerListenerThread.h"

DWORD WINAPI WorkerListenerThread(LPVOID lpParam) {
    PrintDebug("Worker listener started.");

    // Context
    Context* ctx = (Context*)lpParam;

    int iResult;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintInfo("Stop signal received, stopping worker handler.");

            break;
        }

        SOCKET workerSocket = accept(ctx->workerListenSocket, NULL, NULL);
        if (workerSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }

            Sleep(10); // Avoid busy waiting

            continue;
        } else {
            PrintInfo("New worker connected.");

            // Create a structure to pass to the worker thread
            PrintDebug("Creating a new worker handler thread data structure.");
            WorkerHandlerThreadData* handlerThreadData = (WorkerHandlerThreadData*)malloc(sizeof(WorkerHandlerThreadData));
            if (!handlerThreadData) {
                PrintError("Memory allocation failed for thread data.");

                // Close the worker socket
                PrintDebug("Closing worker socket.");
                iResult = closesocket(workerSocket);
                if (iResult == SOCKET_ERROR) {
                    PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
                }

                break;
            }
            handlerThreadData->ctx = ctx; // Pass the context pointer
            handlerThreadData->workerSocket = workerSocket; // Initialize the worker socket
            if (ctx->workerCount < MAX_WORKERS) {
                PrintDebug("Creating a new worker handler thread.");
                ctx->workerHandlerThreads[ctx->workerCount] = CreateThread(NULL, 0, WorkerHandlerThread, handlerThreadData, 0, NULL);
                if (ctx->workerHandlerThreads[ctx->workerCount] == NULL) {
                    PrintError("'CreateThread' failed with error: %d.", GetLastError());

                    // Close the worker socket
                    closesocket(workerSocket);

                    // Free the thread data memory
                    free(handlerThreadData);
                } else {
                    ctx->workerCount++;
                }
            } else {
                PrintWarning("Maximum worker limit reached. Rejecting worker.");

                // Close the worker socket
                closesocket(workerSocket);
            }
        }
    }

    PrintDebug("Worker listener stopped.");

    return TRUE;
}
