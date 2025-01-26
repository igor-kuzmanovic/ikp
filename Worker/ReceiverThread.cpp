#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    Context* context = (Context*)lpParam;

    int iResult = 0;
    MessageBuffer receiveMessageBuffer{};
    MessageBuffer responseMessageBuffer{};
    responseMessageBuffer.message.type = MSG_WORKER_OK;
    responseMessageBuffer.message.payload.healthResponse.isHealthy = TRUE;
    int responseBufferLength = 0;
    int recvResult = 0;
    int sendResult = 0;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping receiver.");

            break;
        }

        // Receive data from server
        recvResult = recv(context->connectSocket, receiveMessageBuffer.buffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received with length %d.", recvResult);

            switch (receiveMessageBuffer.message.type) {
            case MSG_KEY_VALUE_PAIR_STORE_REQUEST:
                PrintInfo("Key-Value Pair: '%s:%s' received from client %d.", receiveMessageBuffer.message.payload.keyValuePairStoreRequest.key, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.value, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.clientId);

                if (HasHashTable(context->hashTable, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.key)) {
                    PrintWarning("Key '%s' already exists in the hash table.", receiveMessageBuffer.message.payload.keyValuePairStoreRequest.key);
                }

                if (SetHashTable(context->hashTable, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.key, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.value) == 1) {
                    PrintDebug("Stored key-value pair. Sending notification to the client %d.", receiveMessageBuffer.message.payload.keyValuePairStoreRequest.clientId);

                    // TODO Remove, for debugging purposes and implement properly later
                    MessageBuffer storedMessageBuffer{};
                    storedMessageBuffer.message.type = MSG_KEY_VALUE_PAIR_STORED;
                    storedMessageBuffer.message.payload.keyValuePairStored.clientId = receiveMessageBuffer.message.payload.keyValuePairStoreRequest.clientId;
                    strcpy_s(storedMessageBuffer.message.payload.keyValuePairStored.key, receiveMessageBuffer.message.payload.keyValuePairStoreRequest.key);
                    
                    send(context->connectSocket, storedMessageBuffer.buffer, BUFFER_SIZE, 0);
                } else {
                    PrintError("Failed to store key-value pair in the hash table.");
                }

                break;

            case MSG_SERVER_SHUTDOWN:
                PrintInfo("Server Shutdown Message.");

                SetFinishSignal(context);

                break;

            case MSG_WORKER_HEALTH_CHECK:
                PrintInfo("Worker Health Check: Worker id = %s.", receiveMessageBuffer.message.payload.healthCheck.workerId);

                // Send a health check response
                sendResult = send(context->connectSocket, responseMessageBuffer.buffer, BUFFER_SIZE, 0);
                if (sendResult > 0) {
                    PrintInfo("Health check response sent.");
                } else if (sendResult == 0) {
                    PrintInfo("Server disconnected.");
                    SetFinishSignal(context);
                } else {
                    if (WSAGetLastError() != WSAEWOULDBLOCK) {
                        // Ignore WSAEWOULDBLOCK, it is not an actual error
                        PrintError("[ReceiverThread] 'send' failed with error: %d.", WSAGetLastError());
                    }
                }

                break;
            default:
                PrintError("Unsupported message type: %d.", receiveMessageBuffer.message.type);

                continue;
            }
        } else if (recvResult == 0) {
            PrintInfo("Server closed the connection.");
            SetFinishSignal(context);

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }

            Sleep(INPUT_WAIT_TIME);

            continue;
        }
    };

    PrintDebug("Receiver stopped.");

    return TRUE;
}