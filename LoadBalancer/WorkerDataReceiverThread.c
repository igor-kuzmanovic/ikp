#include "WorkerDataReceiverThread.h"

DWORD WINAPI WorkerDataReceiverThread(LPVOID lpParam) {

    WorkerDataReceiverThreadData* threadData = (WorkerDataReceiverThreadData*)lpParam;

    SOCKET workerSocket = threadData->workerSocket;
    int workerId = threadData->workerId;
    Context* context = threadData->context;
    int threadIndex = threadData->threadIndex;

    int iResult;
    MessageType messageType;
    uint16_t actualSize;
    char buffer[MAX_MESSAGE_SIZE];
    int recvResult = 0;

    uint32_t readyWorkerId;
    uint16_t peerPort;

    ErrorCode result;
    uint32_t clientId;
    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        recvResult = ProtocolReceive(workerSocket, &messageType, buffer, MAX_MESSAGE_SIZE, &actualSize);

        if (recvResult == 0) {
            PrintDebug("Message received from worker %d: %s (%d bytes)", workerId, GetMessageTypeName(messageType), actualSize);

            switch (messageType) {
            case MSG_WORKER_READY: {
                if (ReceiveWorkerReady(buffer, actualSize, &readyWorkerId, &peerPort) == 0) {
                    PrintInfo("Worker %u announced readiness with peer port %u", readyWorkerId, peerPort);

                    workerId = readyWorkerId;

                    char workerAddress[INET_ADDRSTRLEN];
                    GetWorkerAddress(workerSocket, workerAddress, sizeof(workerAddress));
                    PrintInfo("Worker %d from %s connected", workerId, workerAddress);

                    int addResult = AddWorker(context->workerList, workerSocket, workerId, workerAddress, peerPort);
                    bool isNewWorker = (addResult != -1);

                    if (!isNewWorker) {
                        PrintInfo("Worker %d already exists, updating peer port", workerId);
                        UpdateWorkerPeerPort(context->workerList, workerId, peerPort);
                    } else {
                        PrintInfo("Worker %d successfully added to worker list", workerId);
                    }

                    int result = SetWorkerReady(context->workerList, workerId);
                    if (result > 0) {
                        if (isNewWorker) {
                            BroadcastNewWorkerJoined(context->workerList, workerId);
                        } else {
                            PrintInfo("Worker %d became ready again, no broadcast needed", workerId);
                        }

                        SendWorkerRegistryToSingleWorker(context->workerList, workerId);
                        PrintInfo("Sent registry update to worker %d", workerId);
                    } else if (result == 0) {
                        PrintInfo("Worker %d was already ready", workerId);
                    }

                    PrintInfo("Sent ready acknowledgment to worker %d", workerId);
                } else {
                    PrintError("Failed to parse worker ready message");
                }
                break;
            }

            case MSG_STORE_RESPONSE: {
                if (ReceiveStoreResponse(buffer, actualSize, &result, &clientId, key) == 0) {
                    PrintDebug("Store response from worker %d: result=%s, client=%u, key='%s'",
                        workerId, GetErrorCodeName(result), clientId, key);

                    iResult = PutWorkerResponseQueue(context->workerResponseQueue, workerSocket, buffer, actualSize, messageType, workerId, clientId);
                    if (iResult == 0) {
                        PrintError("Worker response queue is full, notifying the worker that the server is busy.");
                        SendError(workerSocket, ERR_SERVER_BUSY, "Server is busy");

                        Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    } else if (iResult == -1) {
                        SendError(workerSocket, ERR_SERVER_BUSY, "Server error");

                        Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    }
                } else {
                    PrintError("Failed to parse store response message");
                }
                break;
            }

            case MSG_RETRIEVE_RESPONSE: {
                if (ReceiveRetrieveResponse(buffer, actualSize, &result, &clientId, key, value) == 0) {
                    PrintDebug("Retrieve response from worker %d: result=%s, client=%u, key='%s', value='%s'",
                        workerId, GetErrorCodeName(result), clientId, key, value);

                    iResult = PutWorkerResponseQueue(context->workerResponseQueue, workerSocket, buffer, actualSize, messageType, workerId, clientId);
                    if (iResult == 0) {
                        PrintError("Worker response queue is full, notifying the worker that the server is busy.");
                        SendError(workerSocket, ERR_SERVER_BUSY, "Server is busy");

                        Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    } else if (iResult == -1) {
                        PrintError("Failed to put the response in the worker response queue.");
                        SendError(workerSocket, ERR_SERVER_BUSY, "Server error");

                        Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    }
                } else {
                    PrintError("Failed to parse retrieve response message");
                }
                break;
            }
            }
        } else {
            if (recvResult == -2 || recvResult == -3) {
                PrintInfo("Worker %d disconnected or network error (error: %d). Closing connection gracefully.", workerId, recvResult);
            } else {
                PrintError("'ProtocolReceive' failed with error: %d. Closing connection with worker %d.", recvResult, workerId);
            }
            RemoveWorker(context->workerList, workerId);
            break;
        }
    }

    if (workerSocket != INVALID_SOCKET) {
        SendShutdown(workerSocket);

        iResult = SafeCloseSocket(&workerSocket);
        if (iResult != 0) {
            PrintError("SafeCloseSocket failed with error: %d.", iResult);
        }
    } else {
    }

    ReturnWorkerDataReceiverThread(context->workerThreadPool, threadIndex, threadData);

    return TRUE;
}


