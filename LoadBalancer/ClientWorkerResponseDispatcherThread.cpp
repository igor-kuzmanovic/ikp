#include "ClientWorkerResponseDispatcherThread.h"

DWORD WINAPI ClientWorkerResponseDispatcherThread(LPVOID lpParam) {
    PrintDebug("Client worker response dispatcher started.");

    Context* context = (Context*)lpParam;

    int iResult = 0;

    MessageBuffer messageBuffer{};
    messageBuffer.message.type = MSG_KEY_VALUE_PAIR_STORED;
    messageBuffer.message.payload.keyValuePairStored.clientId = 0;
    SOCKET clientSocket = INVALID_SOCKET;
    int sendResult = 0;

    // A variable to store the worker response
    WorkerResponse* response = (WorkerResponse*)malloc(sizeof(WorkerResponse));
    if (response == NULL) {
        PrintError("Failed to allocate memory for the worker response.");

        return FALSE;
    }

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping client worker response dispatcher.");

            break;
        }

        iResult = TakeWorkerResponseQueue(context->workerResponseQueue, response);
        if (iResult == 0) {
            Sleep(WORKER_RESPONSE_QUEUE_EMPTY_SLEEP_TIME);

            continue;
        } else if (iResult < 0) {
            PrintError("Failed to take the worker response.");

            memset(response, 0, sizeof(WorkerResponse));

            continue;
        } else {
            PrintDebug("Got a new worker response.");
        }

        GetClientSocketByClientId(context->clientThreadPool, response->clientId, &clientSocket);
        if (clientSocket == INVALID_SOCKET) {
            PrintError("Failed to get the client socket by client id.");

            memset(response, 0, sizeof(WorkerResponse));

            continue;
        }

        PrintDebug("Sending response to client %d on socket %d.", response->clientId, clientSocket);

        messageBuffer.message.payload.keyValuePairStored.clientId = response->clientId;
        messageBuffer.message.payload.keyValuePairStored.workerId = response->workerId; // TODO Remove this later, client doesn't need this info
        strcpy_s(messageBuffer.message.payload.keyValuePairStored.key, response->data.message.payload.keyValuePairStored.key);

        sendResult = send(clientSocket, messageBuffer.buffer, BUFFER_SIZE, 0);
        if (sendResult > 0) {
            PrintInfo("Message sent to client %d with length %d.", response->clientId, sendResult);
        
            messageBuffer.message.payload.keyValuePairStored.clientId = 0;
            clientSocket = INVALID_SOCKET;
            memset(response, 0, sizeof(WorkerResponse));
        } else if (sendResult == 0) {
            PrintError("Client disconnected.");
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("[ClientWorkerResponseDispatchedThread] 'send' failed with error: %d.", WSAGetLastError());
            }
        
            continue;
        }
    }

    free(response);

    PrintDebug("Client worker response dispatcher stopped.");

    return TRUE;
}
