#include "WorkerDataReceiverThread.h"

DWORD WINAPI WorkerDataReceiverThread(LPVOID lpParam) {
    PrintDebug("Worker data receiver started.");

    WorkerDataReceiverThreadData* threadData = (WorkerDataReceiverThreadData*)lpParam;

    SOCKET workerSocket = threadData->workerSocket;
    int workerId = threadData->workerId;
    Context* context = threadData->context;
    int threadIndex = threadData->threadIndex;

    int iResult;

    MessageBuffer messageBuffer{};
    MessageBuffer serverBusyMessageBuffer{};
    serverBusyMessageBuffer.message.type = MSG_SERVER_BUSY;
    int recvResult = 0;
    int sendResult = 0;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker data receiver.");

            break;
        }

        // Receive data from the worker
        recvResult = recv(workerSocket, messageBuffer.buffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received with length %d.", recvResult);

            switch (messageBuffer.message.type) {
            case MSG_KEY_VALUE_PAIR_STORED:
                iResult = PutWorkerResponseQueue(context->workerResponseQueue, workerSocket, messageBuffer.buffer, recvResult, workerId, messageBuffer.message.payload.keyValuePairStored.clientId);
                if (iResult == 0) {
                    PrintError("Worker request queue is full, notifying the worker that the server is busy.");

                    PrintDebug("Notifying the worker that the server is busy.");
                    send(workerSocket, serverBusyMessageBuffer.buffer, BUFFER_SIZE, 0);

                    PrintDebug("Sleeping for %d ms before accepting new requests.", WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                    Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);

                    continue;
                } else if (iResult == -1) {
                    PrintError("Failed to put the request in the worker request queue.");

                    PrintDebug("Notifying the worker that the server is busy.");
                    send(workerSocket, serverBusyMessageBuffer.buffer, BUFFER_SIZE, 0);

                    PrintDebug("Sleeping for %d ms before accepting new requests.", WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);
                    Sleep(WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME);

                    continue;
                }
            default:
                PrintError("Unknown message type received.");

                continue;
            }
        } else if (recvResult == 0) {
            PrintInfo("Worker disconnected.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }

            continue;
        }
    }

    // Send shutdown notification
    PrintDebug("Notifying worker of server shutdown.");
    sendResult = send(workerSocket, SERVER_SHUTDOWN_MESSAGE, (int)strlen(SERVER_SHUTDOWN_MESSAGE) + 1, 0);
    if (sendResult == SOCKET_ERROR) {
        PrintError("[WorkerDataReceiverThread] 'send' failed with error: %d.", WSAGetLastError());
    }

    // Close the worker socket
    PrintDebug("Closing worker socket.");
    iResult = closesocket(workerSocket);
    if (iResult == SOCKET_ERROR) {
        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
    }

    // Return the thread to the pool
    ReturnWorkerDataReceiverThread(context->workerThreadPool, threadIndex, threadData);

    PrintDebug("Worker data receiver stopped.");

    return TRUE;
}