#include "ReceiverThread.h"
#include "ExportThread.h"
#include "ExportQueue.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    MessageType messageType;
    uint16_t actualSize;
    char buffer[MAX_MESSAGE_SIZE];
    int recvResult = 0;
    int sendResult = 0;
    int socketReady;
    uint32_t clientId;
    int broadcastCount = 0;
    int activeCount = 0;
    int removedCount = 0;
    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];
    uint32_t totalWorkers;
    ErrorCode errorCode;
    char errorMessage[MAX_ERROR_MSG_SIZE + 1];
    uint32_t workerId;
    char address[256];
    uint16_t port;
    uint8_t shouldExportData;

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        socketReady = IsSocketReadyToRead(context->connectSocket, 100);

        if (socketReady == 1) {
            recvResult = ProtocolReceive(context->connectSocket, &messageType, buffer, MAX_MESSAGE_SIZE, &actualSize);

            if (recvResult == 0) {
                if (messageType == MSG_ERROR) {
                    ErrorCode errorCode = (ErrorCode)buffer[0];
                    PrintDebug("Message received: %s (%d bytes), error code: %s",
                        GetMessageTypeName(messageType), actualSize, GetErrorCodeName(errorCode));
                } else {
                    PrintDebug("Message received: %s (%d bytes)", GetMessageTypeName(messageType), actualSize);
                }

                switch (messageType) {
                case MSG_STORE_REQUEST: {
                    if (ReceiveStoreRequest(buffer, actualSize, &clientId, key, value) == 0) {
                        PrintDebug("Stored: '%s':'%s' for client %u", key, value, clientId);

                        if (HasHashTable(context->hashTable, key)) {
                            PrintWarning("Key '%s' already exists in the hash table.", key);
                        }

                        if (SetHashTable(context->hashTable, key, value) == 1) {
                            if (context->peerManager != NULL) {
                                broadcastCount = BroadcastData(context->peerManager, key, value);
                                if (broadcastCount > 0) {
                                    PrintDebug("Notified %d peer workers about new data", broadcastCount);
                                }
                            }

                            EnterCriticalSection(&context->lock);
                            sendResult = SendStoreResponse(context->connectSocket, ERR_NONE, clientId, key);
                            LeaveCriticalSection(&context->lock);

                            if (sendResult <= 0) {
                                PrintError("Failed to send store response to LoadBalancer.");
                                continue;
                            }
                        } else {
                            PrintError("Failed to store key-value pair in hash table.");

                            EnterCriticalSection(&context->lock);
                            sendResult = SendStoreResponse(context->connectSocket, ERR_NETWORK_ERROR, clientId, key);
                            LeaveCriticalSection(&context->lock);

                            if (sendResult <= 0) {
                                PrintError("Failed to send error response to LoadBalancer.");
                                continue;
                            }
                        }
                    } else {
                        PrintError("Failed to parse store request message");
                    }
                    break;
                }

                case MSG_RETRIEVE_REQUEST: {
                    if (ReceiveRetrieveRequest(buffer, actualSize, &clientId, key) == 0) {
                        PrintDebug("Retrieve: '%s' for client %u", key, clientId);

                        char* valueToSend = NULL;
                        if (GetHashTable(context->hashTable, key, &valueToSend) == 1) {
                            EnterCriticalSection(&context->lock);
                            sendResult = SendRetrieveResponse(context->connectSocket, ERR_NONE, clientId, key, valueToSend);
                            LeaveCriticalSection(&context->lock);
                            free(valueToSend);
                            if (sendResult <= 0) {
                                PrintError("Failed to send retrieve response to LoadBalancer (error: %d).", sendResult);
                                continue;
                            }
                        } else {
                            EnterCriticalSection(&context->lock);
                            sendResult = SendRetrieveResponse(context->connectSocket, ERR_KEY_NOT_FOUND, clientId, key, "");
                            LeaveCriticalSection(&context->lock);

                            if (sendResult <= 0) {
                                PrintError("Failed to send not found response to LoadBalancer (error: %d).", sendResult);
                                continue;
                            }
                            PrintInfo("Key '%s' not found for client %u", key, clientId);
                        }
                    } else {
                        PrintError("Failed to parse retrieve request message");
                    }
                    break;
                }

                case MSG_SHUTDOWN:
                    PrintInfo("Server shutdown message received.");
                    SetFinishSignal(context);
                    break;

                case MSG_WORKER_REGISTRY_START: {

                    if (ReceiveWorkerRegistryStart(buffer, actualSize, &totalWorkers) == 0) {
                        PrintDebug("Worker registry update starting: %u workers", totalWorkers);
                    } else {
                        PrintError("Failed to parse worker registry start message");
                    }
                    break;
                }

                case MSG_WORKER_ENTRY: {
                    if (ReceiveWorkerEntry(buffer, actualSize, &workerId, address, &port, &shouldExportData) == 0) {
                        if (context->peerManager != NULL) {
                            AddPeer(context->peerManager, workerId, address, port);

                            if (shouldExportData) {
                                PrintInfo("LoadBalancer requested data export to worker %u", workerId);

                                if (AddExportRequest(context->exportQueue, workerId, address, port) == 0) {
                                    PrintInfo("Export request queued for worker %u", workerId);
                                } else {
                                    PrintError("Failed to queue export request for worker %u", workerId);
                                }
                            }
                        }
                    } else {
                        PrintError("Failed to parse worker entry message");
                    }
                    break;
                }

                case MSG_WORKER_REGISTRY_END: {
                    PrintDebug("Worker registry update completed");

                    if (context->peerManager != NULL) {
                        activeCount = 0;
                        removedCount = 0;
                        FinalizePeerRegistryUpdate(context->peerManager, &activeCount, &removedCount);
                        PrintDebug("Peer registry updated: %d active peers, %d removed", activeCount, removedCount);
                    }
                    break;
                }

                case MSG_ERROR: {
                    if (ReceiveError(buffer, actualSize, &errorCode, errorMessage) == 0) {
                        PrintInfo("LoadBalancer message: %s (code: %s)",
                            errorMessage, GetErrorCodeName(errorCode));
                    } else {
                        PrintError("Failed to parse error message");
                    }
                    break;
                }
                }
            } else {
                if (recvResult == -2 || recvResult == -3) {
                    PrintInfo("Worker disconnected or network error (error: %d). Closing connection gracefully.", recvResult);
                } else {
                    PrintError("'ProtocolReceive' failed with error: %d.", recvResult);
                }
                SetFinishSignal(context);
                break;
            }
        }
    }

    return TRUE;
}


