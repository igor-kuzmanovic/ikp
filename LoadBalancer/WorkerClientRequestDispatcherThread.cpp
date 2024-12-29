#include "WorkerClientRequestDispatcherThread.h"

DWORD WINAPI WorkerClientRequestDispatcherThread(LPVOID lpParam) {
    PrintDebug("Worker client request dispatcher started.");

    Context* ctx = (Context*)lpParam;

    int iResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    // A variable to store the client request
    ClientRequest* request = NULL;

    // A variable to store the worker
    WorkerNode* worker = NULL;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker client request dispatcher.");

            break;
        }

        if (request == NULL) {
            request = (ClientRequest*)malloc(sizeof(ClientRequest));
            if (request == NULL) {
                PrintError("Failed to allocate memory for the client request.");
            } else {
                PrintDebug("Getting next client request.");
                iResult = TakeClientRequestQueue(ctx->clientRequestQueue, request);
                if (iResult == 0) {
                    PrintDebug("Client request queue is empty.");

                    free(request);
                    request = NULL;
                } else if (iResult < 0) {
                    PrintError("Failed to take the client request.");

                    free(request);
                    request = NULL;
                } else {
                    PrintDebug("Got a new client request.");
                }
            }
        }

        if (worker == NULL) {
            worker = (WorkerNode *)malloc(sizeof(WorkerNode));
            if (worker == NULL) {
                PrintError("Failed to allocate memory for the worker node.");
            } else {
                PrintDebug("Getting next worker.");
                iResult = GetNextWorker(ctx->workerList, worker);
                if (iResult == 0) {
                    PrintDebug("Worker list is empty.");

                    free(worker);
                    worker = NULL;
                } else if (iResult < 0) {
                    PrintError("Failed to get the next worker.");

                    free(worker);
                    worker = NULL;
                } else {
                    PrintDebug("Got the next worker.");
                }
            }
        }

        // Send the client request to the worker
        if (request != NULL && worker != NULL) {
            PrintInfo("Dispatching client request from client socket %d to worker %d", request->clientSocket, worker->socket);

            sendResult = send(worker->socket, request->data, BUFFER_SIZE, 0);
            if (sendResult > 0) {
                PrintInfo("Message sent to worker %d: '%s' with length %d.", worker->socket, request->data, sendResult);

                free(request);
                request = NULL;
                free(worker);
                worker = NULL;
            } else if (sendResult == 0) {
                PrintError("Worker disconnected.");

                RemoveWorker(ctx->workerList, worker->socket);

                free(worker);
                worker = NULL;
            } else {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    // Ignore WSAEWOULDBLOCK, it is not an actual error
                    PrintError("'send' failed with error: %d.", WSAGetLastError());

                    PrintError("Worker disconnected.");

                    RemoveWorker(ctx->workerList, worker->socket);

                    free(worker);
                    worker = NULL;
                }
            }
        } else if (request != NULL) {
            PrintDebug("Holding the request until a worker is available.");
        } else if (worker != NULL) {
            PrintDebug("Holding the worker until a request is available.");
        }
    }

    PrintDebug("Worker client request dispatcher stopped.");

    return TRUE;
}
