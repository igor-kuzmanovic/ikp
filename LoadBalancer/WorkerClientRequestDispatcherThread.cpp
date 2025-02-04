#include "WorkerClientRequestDispatcherThread.h"

DWORD WINAPI WorkerClientRequestDispatcherThread(LPVOID lpParam) {
    PrintDebug("Worker client request dispatcher started.");

    Context* context = (Context*)lpParam;

    int iResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    // A variable to store the client request
    ClientRequest* request = (ClientRequest*)malloc(sizeof(ClientRequest));
    if (request == NULL) {
        PrintError("Failed to allocate memory for the client request.");

        return FALSE;
    }

    // A variable to store the worker
    WorkerNode* worker = (WorkerNode*)malloc(sizeof(WorkerNode));
    if (worker == NULL) {
        PrintError("Failed to allocate memory for the worker node.");

        free(request);

        return FALSE;
    }

    // Flags to check if the request and worker are set
    bool hasRequest = false;
    bool hasWorker = false;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker client request dispatcher.");

            break;
        }

        // Get the next client request if needed
        if (!hasRequest) {
            iResult = TakeClientRequestQueue(context->clientRequestQueue, request);
            if (iResult == 0) {
                Sleep(CLIENT_REQUEST_QUEUE_EMPTY_SLEEP_TIME);

                continue;
            } else if (iResult < 0) {
                PrintError("Failed to take the client request.");

                memset(request, 0, sizeof(ClientRequest));

                continue;
            } else {
                PrintDebug("Got a new client request.");
                hasRequest = true;
            }
        }

        // Get the next worker if needed
        if (!hasWorker) {
            iResult = GetNextWorker(context->workerList, worker);
            if (iResult == 0) {
                Sleep(WORKER_LIST_EMPTY_SLEEP_TIME);

                continue;
            } else if (iResult < 0) {
                PrintError("Failed to get the next worker.");

                memset(worker, 0, sizeof(WorkerNode));

                continue;
            } else {
                PrintDebug("Got the next worker.");
                hasWorker = true;
            }
        }

        // Send the client request to the worker
        if (hasRequest && hasWorker) {
            PrintInfo("Dispatching client request from client socket %d to worker %d", request->clientSocket, worker->socket);

            sendResult = send(worker->socket, request->data, (int)strlen(request->data) + 1, 0);
            if (sendResult > 0) {
                PrintInfo("Message sent to worker %d: '%s' with length %d.", worker->socket, request->data, sendResult);

                memset(request, 0, sizeof(ClientRequest));
                hasRequest = false;

                memset(worker, 0, sizeof(WorkerNode));
                hasWorker = false;
            } else if (sendResult == 0) {
                PrintError("Worker disconnected.");

                memset(worker, 0, sizeof(WorkerNode));
                hasWorker = false;
            } else {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    // Ignore WSAEWOULDBLOCK, it is not an actual error
                    PrintError("[WorkerClientRequestDispatchedThread] 'send' failed with error: %d.", WSAGetLastError());

                    memset(worker, 0, sizeof(WorkerNode));
                    hasWorker = false;
                }

                continue;
            }
        } else if (hasRequest) {
            PrintDebug("Holding the request until a worker is available.");
        } else if (hasWorker) {
            PrintDebug("Holding the worker until a request is available.");
        }
    }

    free(request);
    free(worker);

    PrintDebug("Worker client request dispatcher stopped.");

    return TRUE;
}
