#include "WorkerClientRequestDispatcherThread.h"

DWORD WINAPI WorkerClientRequestDispatcherThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    int iResult = 0;
    int sendResult = 0;

    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];

    ClientRequest* request = (ClientRequest*)malloc(sizeof(ClientRequest));
    if (request == NULL) {
        PrintError("Failed to allocate memory for the client request.");
        return FALSE;
    }

    WorkerNode* worker = (WorkerNode*)malloc(sizeof(WorkerNode));
    if (worker == NULL) {
        PrintError("Failed to allocate memory for the worker node.");
        free(request);
        return FALSE;
    }

    int hasRequest = 0;
    int hasWorker = 0;

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

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
                hasRequest = true;
            }
        }

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
                hasWorker = true;
            }
        }

        if (hasRequest && hasWorker) {
            PrintDebug("Dispatching client request from client socket %d to worker socket %d", request->clientSocket, worker->workerSocket);

            MessageType messageType = request->messageType;

            if (messageType == MSG_PUT) {
                if (ReceivePut(request->data, request->dataSize, key, value) == 0) {
                    sendResult = SendStoreRequest(worker->workerSocket, request->clientId, key, value);
                    if (sendResult > 0) {
                        PrintDebug("Store request sent to worker %d: '%s:%s'",
                            worker->workerId, key, value);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;
                    } else if (sendResult == -2 || sendResult == -3) {
                        PrintInfo("Connection to worker %d lost (error: %d), marking worker as disconnected", worker->workerId, sendResult);
                        SetWorkerDisconnected(context->workerList, worker->workerId);

                        ReturnClientRequestQueue(context->clientRequestQueue, request);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;

                        Sleep(LB_QUEUE_RETRY_DELAY);
                    } else {
                        PrintError("Failed to send store request to worker %d (error: %d)", worker->workerId, sendResult);
                        ReturnClientRequestQueue(context->clientRequestQueue, request);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;
                    }
                } else {
                    PrintError("Failed to parse PUT request data");
                    memset(request, 0, sizeof(ClientRequest));
                    hasRequest = false;
                }
            } else if (messageType == MSG_GET) {
                if (ReceiveGet(request->data, request->dataSize, key) == 0) {
                    sendResult = SendRetrieveRequest(worker->workerSocket, request->clientId, key);
                    if (sendResult > 0) {
                        PrintDebug("Retrieve request sent to worker %d: '%s'",
                            worker->workerId, key);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;
                    } else if (sendResult == -2 || sendResult == -3) {
                        PrintInfo("Connection to worker %d lost (error: %d), marking worker as disconnected", worker->workerId, sendResult);
                        SetWorkerDisconnected(context->workerList, worker->workerId);

                        ReturnClientRequestQueue(context->clientRequestQueue, request);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;

                        Sleep(LB_QUEUE_RETRY_DELAY);
                    } else {
                        PrintError("Failed to send retrieve request to worker %d (error: %d)", worker->workerId, sendResult);
                        ReturnClientRequestQueue(context->clientRequestQueue, request);

                        memset(request, 0, sizeof(ClientRequest));
                        hasRequest = false;

                        memset(worker, 0, sizeof(WorkerNode));
                        hasWorker = false;
                    }
                } else {
                    PrintError("Failed to parse GET request data");
                    memset(request, 0, sizeof(ClientRequest));
                    hasRequest = false;
                }
            }
        } else if (hasRequest) {
        } else if (hasWorker) {
        }
    }

    free(request);
    free(worker);

    return TRUE;
}


