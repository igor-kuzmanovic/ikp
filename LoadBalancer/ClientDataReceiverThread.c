#include "ClientDataReceiverThread.h"

DWORD WINAPI ClientDataReceiverThread(LPVOID lpParam) {

    ClientDataReceiverThreadData* threadData = (ClientDataReceiverThreadData*)lpParam;

    SOCKET clientSocket = threadData->clientSocket;
    int clientId = threadData->clientId;
    Context* context = threadData->context;
    int threadIndex = threadData->threadIndex;

    int iResult;
    MessageType messageType;
    uint16_t actualSize;
    char buffer[MAX_MESSAGE_SIZE];
    int recvResult = 0;
    int sendResult = 0;

    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        recvResult = ProtocolReceive(clientSocket, &messageType, buffer, MAX_MESSAGE_SIZE, &actualSize);
        if (recvResult == 0) {
            PrintDebug("Message received: %s (%d bytes)", GetMessageTypeName(messageType), actualSize);

            switch (messageType) {
            case MSG_PUT: {
                if (ReceivePut(buffer, actualSize, key, value) == 0) {
                    PrintDebug("PUT request: '%s':'%s' from client %d", key, value, clientId);
                    if (GetWorkerCount(context->workerList) == 0) {
                        PrintInfo("No workers available, sending error to client");
                        SendError(clientSocket, ERR_SERVER_BUSY, "No workers available");
                        continue;
                    }

                    iResult = PutClientRequestQueue(context->clientRequestQueue, clientSocket, buffer, actualSize, messageType, clientId);
                    if (iResult == 0) {
                        PrintError("Client request queue is full, notifying the client that the server is busy.");
                        SendError(clientSocket, ERR_SERVER_BUSY, "Server is busy, please try again later");

                        Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    } else if (iResult == -1) {
                        PrintError("Failed to put the request in the client request queue.");
                        SendError(clientSocket, ERR_SERVER_BUSY, "Server error, please try again later");

                        Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);                        continue;
                    }
                } else {
                    PrintError("Failed to parse PUT message");
                    SendError(clientSocket, ERR_PROTOCOL_ERROR, "Invalid PUT message format");
                }
                break;
            }

            case MSG_GET: {
                if (ReceiveGet(buffer, actualSize, key) == 0) {
                    PrintDebug("GET request: '%s' from client %d", key, clientId);

                    if (GetWorkerCount(context->workerList) == 0) {
                        PrintInfo("No workers available, sending error to client");
                        SendError(clientSocket, ERR_SERVER_BUSY, "No workers available");
                        continue;
                    }

                    iResult = PutClientRequestQueue(context->clientRequestQueue, clientSocket, buffer, actualSize, messageType, clientId);
                    if (iResult == 0) {
                        PrintError("Client request queue is full, notifying the client that the server is busy.");
                        SendError(clientSocket, ERR_SERVER_BUSY, "Server is busy, please try again later");

                        Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    } else if (iResult == -1) {
                        PrintError("Failed to put the request in the client request queue.");
                        SendError(clientSocket, ERR_SERVER_BUSY, "Server error, please try again later");

                        Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);
                        continue;
                    }
                } else {
                    PrintError("Failed to parse GET message");
                    SendError(clientSocket, ERR_PROTOCOL_ERROR, "Invalid GET message format");
                }
                break;
            }
            }
        } else {
            if (recvResult == -2 || recvResult == -3) {
                PrintInfo("Client %d disconnected or network error (error: %d). Closing connection gracefully.", clientId, recvResult);
            } else {
                PrintError("'ProtocolReceive' failed with error: %d. Closing connection with client %d.", recvResult, clientId);
            }
            break;
        }
    }

    if (clientSocket != INVALID_SOCKET) {
        SendShutdown(clientSocket);

        iResult = SafeCloseSocket(&clientSocket);
        if (iResult != 0) {
            PrintError("SafeCloseSocket failed with error: %d.", iResult);
        }
    } else {
    }

    ReturnClientDataReceiverThread(context->clientThreadPool, threadIndex, threadData);

    return TRUE;
}


