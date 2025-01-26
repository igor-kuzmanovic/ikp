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
            case MSG_KEY_VALUE_PAIR:
                PrintInfo("Key-Value Pair: '%s:%s'.", receiveMessageBuffer.message.payload.keyValuePair.key, receiveMessageBuffer.message.payload.keyValuePair.value);

                break;

            case MSG_SERVER_SHUTDOWN:
                PrintInfo("Server Shutdown Message.");

                break;

            case MSG_WORKER_HEALTH_CHECK:
                PrintInfo("Worker Health Check: Worker ID = %s.", receiveMessageBuffer.message.payload.healthCheck.workerId);

                break;

            case MSG_WORKER_OK:
                PrintInfo("Worker Health Response: Is Healthy = %d.", receiveMessageBuffer.message.payload.healthResponse.isHealthy);

                break;

            default:
                PrintError("Unsupported message type: %d.", receiveMessageBuffer.message.type);

                break;
            }

            switch (receiveMessageBuffer.message.type) {
            case MSG_KEY_VALUE_PAIR:
                if (HasHashTable(context->hashTable, receiveMessageBuffer.message.payload.keyValuePair.key)) {
                    PrintWarning("Key '%s' already exists in the hash table.", receiveMessageBuffer.message.payload.keyValuePair.key);
                }

                if (SetHashTable(context->hashTable, receiveMessageBuffer.message.payload.keyValuePair.key, receiveMessageBuffer.message.payload.keyValuePair.value) == 1) {
                    PrintDebug("Stored key-value pair. Sending notification.");

                    // TODO Remove, for debugging purposes and implement properly later
                    send(context->connectSocket, WORKER_OK, (int)strlen(WORKER_OK) + 1, 0);
                } else {
                    PrintError("Failed to store key-value pair in the hash table.");
                }

                break;

            case MSG_SERVER_SHUTDOWN:
                SetFinishSignal(context);

                break;

            case MSG_WORKER_HEALTH_CHECK:
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
                break;
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