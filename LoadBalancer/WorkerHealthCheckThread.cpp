#include "WorkerHealthCheckThread.h"

DWORD WINAPI WorkerHealthCheckThread(LPVOID lpParam) {
    PrintDebug("[WorkerHealthCheckThread] Thread started.");

    Context* context = (Context*)lpParam;

    int iResult;
    char sendBuffer[BUFFER_SIZE];
    int sendBufferLength;
    int sendResult;
    WorkerNode* worker = NULL;
    Message message{};

    while (true) {
        Sleep(1000); // TODO Remove, for testing purposes
        continue; // TODO Remove, for testing purposes

        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("[WorkerHealthCheckThread] Stop signal received, stopping thread.");

            break;
        }

        // If the worker list is NULL, stop the thread
        if (context->workerList == NULL) {
            break;
        } else if (GetWorkerCount(context->workerList) == 0) {
            Sleep(WORKER_HEALTH_CHECK_INTERVAL);

            continue;
        }

        PrintInfo("[WorkerHealthCheckThread] Checking worker health.");

        iResult = IterateWorkersOnce(context->workerList, &worker);
        if (iResult < 0) {
            PrintError("[WorkerHealthCheckThread] Failed to iterate workers.");

            worker = NULL;

            break;
        }

        while (worker != NULL) {
            // Check if the worker is still alive
            message.type = MSG_WORKER_HEALTH_CHECK;
            sendBufferLength = SerializeMessage(&message, sendBuffer);
            if (sendBufferLength <= 0) {
                PrintError("[WorkerHealthCheckThread] Failed to serialize the message.");

                break;
            }

            sendResult = send(worker->workerSocket, sendBuffer, sendBufferLength, 0);
            if (sendResult > 0) {
                PrintInfo("[WorkerHealthCheckThread] Health check sent to worker: id %d, socket %d.", worker->workerId, worker->workerSocket);
            } else if (sendResult == 0) {
                PrintInfo("[WorkerHealthCheckThread] Worker disconnected.");

                iResult = RemoveWorker(context->workerList, worker->workerId);
                if (iResult < 0) {
                    PrintError("[WorkerHealthCheckThread] Failed to remove the worker from the list.");
                }

                if (worker->workerSocket != INVALID_SOCKET) {
                    iResult = closesocket(worker->workerSocket);
                    if (iResult == SOCKET_ERROR) {
                        PrintInfo("[WorkerHealthCheckThread] 'closesocket' failed with error: %d.", WSAGetLastError());
                    }
                }

                worker = NULL;

                break;
            } else {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    // Ignore WSAEWOULDBLOCK, it is not an actual error
                    PrintInfo("[WorkerHealthCheckThread] 'send' failed with error: %d.", WSAGetLastError());

                    iResult = RemoveWorker(context->workerList, worker->workerId);
                    if (iResult < 0) {
                        PrintError("[WorkerHealthCheckThread] Failed to remove the worker from the list.");
                    }

                    if (worker->workerSocket != INVALID_SOCKET) {
                        iResult = closesocket(worker->workerSocket);
                        if (iResult == SOCKET_ERROR) {
                            PrintInfo("[WorkerHealthCheckThread] 'closesocket' failed with error: %d.", WSAGetLastError());
                        }
                    }

                    worker = NULL;

                    break;
                }
            }

            iResult = IterateWorkersOnce(context->workerList, &worker);
            if (iResult < 0) {
                PrintError("[WorkerHealthCheckThread] Failed to iterate workers.");

                break;
            }
        }

        Sleep(WORKER_HEALTH_CHECK_INTERVAL);
    }

    PrintDebug("[WorkerHealthCheckThread] Thread stopped.");

    return TRUE;
}
