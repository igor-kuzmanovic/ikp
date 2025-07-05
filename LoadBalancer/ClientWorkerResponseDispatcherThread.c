#include "ClientWorkerResponseDispatcherThread.h"

DWORD WINAPI ClientWorkerResponseDispatcherThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    int iResult = 0;
    SOCKET clientSocket = INVALID_SOCKET;

    ErrorCode result;
    uint32_t clientId;
    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];

    WorkerResponse* response = (WorkerResponse*)malloc(sizeof(WorkerResponse));
    if (response == NULL) {
        PrintError("Failed to allocate memory for the worker response.");
        return FALSE;
    }

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
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
        }

        GetClientSocketByClientId(context->clientThreadPool, response->clientId, &clientSocket);
        if (clientSocket == INVALID_SOCKET) {
            PrintError("Failed to get the client socket by client id.");
            memset(response, 0, sizeof(WorkerResponse));
            continue;
        }

        MessageType responseType = response->messageType;

        switch (responseType) {
        case MSG_STORE_RESPONSE: {
            if (ReceiveStoreResponse(response->data, response->dataSize, &result, &clientId, key) == 0) {
                if (SendPutResponse(clientSocket, result, key) > 0) {
                    PrintDebug("PUT response sent to client %d for key '%s': %s",
                        clientId, key, GetErrorCodeName(result));
                } else {
                    PrintError("Failed to send PUT response to client %d", clientId);
                }
            } else {
                PrintError("Failed to parse store response data");
                SendError(clientSocket, ERR_PROTOCOL_ERROR, "Invalid worker response");
            }
            break;
        }

        case MSG_RETRIEVE_RESPONSE: {
            if (ReceiveRetrieveResponse(response->data, response->dataSize, &result, &clientId, key, value) == 0) {
                if (SendGetResponse(clientSocket, result, key, value) > 0) {
                    PrintDebug("GET response sent to client %d for key '%s': '%s'",
                        clientId, key, value);
                } else {
                    PrintError("Failed to send GET response to client %d", clientId);
                }
            } else {
                PrintError("Failed to parse retrieve response data");
                SendError(clientSocket, ERR_PROTOCOL_ERROR, "Invalid worker response");
            }
            break;
        }

        default:
            PrintError("Unsupported worker response type: %s", GetMessageTypeName(responseType));
            SendError(clientSocket, ERR_PROTOCOL_ERROR, "Unsupported response type");
            break;
        }

        memset(response, 0, sizeof(WorkerResponse));
    }

    free(response);

    return TRUE;
}


