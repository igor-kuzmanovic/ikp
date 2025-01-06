#include "WorkerHealthCheckThread.h"

DWORD WINAPI WorkerHealthCheckThread(LPVOID lpParam) {
    PrintDebug("[WorkerHealthCheckThread] Thread started.");

    Context* ctx = (Context*)lpParam;

    int iResult;

    // A variable to store the result of send
    int sendResult;

    // A pointer to the worker
    WorkerNode* worker = NULL;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("[WorkerHealthCheckThread] Stop signal received, stopping thread.");

            break;
        }

        // If the worker list is NULL, stop the thread
        if (ctx->workerList == NULL) {
            break;
        } else if (GetWorkerCount(ctx->workerList) == 0) {
            Sleep(WORKER_HEALTH_CHECK_INTERVAL);

            continue;
        }

        PrintInfo("[WorkerHealthCheckThread] Checking worker health.");

        iResult = IterateWorkersOnce(ctx->workerList, &worker);
        if (iResult < 0) {
            PrintError("[WorkerHealthCheckThread] Failed to iterate workers.");

            worker = NULL;

            break;
        }

        while (worker != NULL) {
            // Check if the worker is still alive
            sendResult = send(worker->socket, WORKER_HEALTH_CHECK_MESSAGE, (int)strlen(WORKER_HEALTH_CHECK_MESSAGE) + 1, 0);
            if (sendResult > 0) {
                PrintInfo("[WorkerHealthCheckThread] Health check sent to worker: id %d, socket %d.", worker->id, worker->socket);
            } else if (sendResult == 0) {
                PrintInfo("[WorkerHealthCheckThread] Worker disconnected.");

                iResult = RemoveWorker(ctx->workerList, worker->id);
                if (iResult < 0) {
                    PrintError("[WorkerHealthCheckThread] Failed to remove the worker from the list.");
                }

                if (worker->socket != INVALID_SOCKET) {
                    iResult = closesocket(worker->socket);
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

                    iResult = RemoveWorker(ctx->workerList, worker->id);
                    if (iResult < 0) {
                        PrintError("[WorkerHealthCheckThread] Failed to remove the worker from the list.");
                    }

                    if (worker->socket != INVALID_SOCKET) {
                        iResult = closesocket(worker->socket);
                        if (iResult == SOCKET_ERROR) {
                            PrintInfo("[WorkerHealthCheckThread] 'closesocket' failed with error: %d.", WSAGetLastError());
                        }
                    }

                    worker = NULL;

                    break;
                }
            }

            iResult = IterateWorkersOnce(ctx->workerList, &worker);
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
