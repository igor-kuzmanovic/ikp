#include "WorkerList.h"

int InitializeWorkerList(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'InitializeWorkerList'.");
        return -1;
    }

    InitializeCriticalSection(&list->lock);

    list->head = NULL;
    list->current = NULL;
    list->count = 0;

    list->notEmpty = CreateSemaphore(NULL, 0, MAX_WORKERS, NULL);
    if (list->notEmpty == NULL) {
        PrintCritical("Failed to create a 'notEmpty' semaphore for the worker list.");
        return GetLastError();
    }

    list->notFull = CreateSemaphore(NULL, MAX_WORKERS, MAX_WORKERS, NULL);
    if (list->notFull == NULL) {
        PrintCritical("Failed to create a 'notFull' semaphore for the worker list.");
        return GetLastError();
    }

    return 0;
}

int DestroyWorkerList(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'DestroyWorkerList'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* temp = list->head;
    if (temp) {
        do {
            WorkerNode* next = temp->next;
            
            if (temp->workerSocket != INVALID_SOCKET) {
                SafeCloseSocket(&temp->workerSocket);
            }
            
            free(temp);
            temp = next;
        } while (temp != list->head);
    }

    list->head = NULL;
    list->current = NULL;
    list->count = 0;

    CloseHandle(list->notFull);
    CloseHandle(list->notEmpty);

    LeaveCriticalSection(&list->lock);
    DeleteCriticalSection(&list->lock);

    return 0;
}

int AddWorker(WorkerList* list, const SOCKET workerSocket, const int workerId, const char* workerAddress, const int workerPeerPort) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'AddWorker'.");
        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid socket provided to 'AddWorker'.");
        return -1;
    }

    if (workerAddress == NULL) {
        PrintError("Invalid worker address provided to 'AddWorker'.");
        return -1;
    }

    DWORD waitResult = WaitForSingleObject(list->notFull, WORKER_LIST_ADD_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to add the worker in the worker list.");

        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (list->count >= MAX_WORKERS) {
        PrintError("Worker list is at maximum capacity (%d) after semaphore signal.", MAX_WORKERS);

        LeaveCriticalSection(&list->lock);

        return -1;
    }

    WorkerNode* newNode = (WorkerNode*)malloc(sizeof(WorkerNode));
    if (!newNode) {
        PrintError("Failed to allocate memory for a new worker node.");

        LeaveCriticalSection(&list->lock);

        return -1;
    }

    newNode->workerId = workerId;
    newNode->workerSocket = workerSocket;
    strncpy_s(newNode->workerAddress, sizeof(newNode->workerAddress), workerAddress, _TRUNCATE);
    newNode->workerPeerPort = workerPeerPort;
    newNode->isConnected = 1; 
    newNode->isReady = 0; 

    if (list->head == NULL) {
        list->head = newNode;
        list->current = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        WorkerNode* tail = list->head->prev;
        tail->next = newNode;
        newNode->prev = tail;
        newNode->next = list->head;
        list->head->prev = newNode;
    }
    list->count++;

    LeaveCriticalSection(&list->lock);

    ReleaseSemaphore(list->notEmpty, 1, NULL);

    return 0;
}

int RemoveWorker(WorkerList* list, const int workerId) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'RemoveWorker'.");

        return -1;
    }

    if (workerId <= 0) {
        PrintError("Invalid id provided to 'RemoveWorker'.");

        return -1;
    }

    DWORD waitResult = WaitForSingleObject(list->notEmpty, WORKER_LIST_REMOVE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to remove the worker from the worker list.");

        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* temp = list->head;
    int isFound = 0;  

    if (temp) {
        do {
            if (temp->workerId == workerId) {
                isFound = true;

                if (temp == list->head && temp->next == list->head) {
                    list->head = NULL;
                    list->current = NULL;
                } else {
                    if (temp == list->head) {
                        list->head = temp->next;
                    }
                    temp->prev->next = temp->next;
                    temp->next->prev = temp->prev;

                    if (temp == list->current) {
                        list->current = temp->next;
                    }
                }

                free(temp);
                list->count--;

                break;
            }

            temp = temp->next;
        } while (temp != list->head);
    }

    LeaveCriticalSection(&list->lock);

    ReleaseSemaphore(list->notFull, 1, NULL);

    if (!isFound) {
        PrintWarning("Worker with id %d not found.", workerId);

        return -1;
    }

    return 0;
}

int GetNextWorker(WorkerList* list, WorkerNode* worker) {
    if (list == NULL) {
        PrintError("Invalid 'WorkerList' pointer provided to 'GetNextWorker'.");

        return -1;
    }

    if (worker == NULL) {
        PrintError("Invalid 'WorkerNode' pointer provided to 'GetNextWorker'.");

        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (list->count == 0 || list->current == NULL) {
        PrintError("Worker list is empty after semaphore signal.");

        LeaveCriticalSection(&list->lock);

        return -1;
    }

    WorkerNode* currentNode = list->current;
    
    int attempts = 0;
    while (attempts < list->count) {
        if (currentNode->isConnected && currentNode->isReady) {
            break;
        }
        
        currentNode = currentNode->next;
        attempts++;
    }
    
    if (attempts >= list->count) {
        PrintError("No ready workers available.");
        LeaveCriticalSection(&list->lock);
        return -1;
    }
    
    list->current = currentNode->next;
    worker->workerId = currentNode->workerId;
    worker->workerSocket = currentNode->workerSocket;

    LeaveCriticalSection(&list->lock);

    return 1;
}

int IterateWorkersOnce(WorkerList* list, WorkerNode** iterator) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'IterateWorkers'.");

        return -1;
    }

    int workerId = 0;

    EnterCriticalSection(&list->lock);

    if (list->count == 0) {

        LeaveCriticalSection(&list->lock);

        return workerId;
    }

    if (*iterator == NULL) {
        *iterator = list->head;
        workerId = (*iterator)->workerId;
    } else if ((*iterator)->next == list->head) {
        *iterator = NULL;
        workerId = 0;
    } else {
        *iterator = (*iterator)->next;
        workerId = (*iterator)->workerId;
    }

    LeaveCriticalSection(&list->lock);

    return workerId;
}

int GetWorkerCount(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'GetWorkerCount'.");

        return -1;
    }

    EnterCriticalSection(&list->lock);

    int count = list->count;

    LeaveCriticalSection(&list->lock);

    return count;
}

int BroadcastWorkerRegistryUpdate(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'BroadcastWorkerRegistryUpdate'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (list->count == 0) {
        LeaveCriticalSection(&list->lock);
        PrintInfo("No workers to broadcast registry update to.");
        return 0;
    }

    int workerIds[MAX_WORKERS];
    char addresses[MAX_WORKERS][256];
    int ports[MAX_WORKERS];
    int workerCount = 0;
    
    
    WorkerNode* current = list->head;
    
    if (current != NULL) {
        do {
            if (workerCount < MAX_WORKERS) {
                workerIds[workerCount] = current->workerId;
                strncpy_s(addresses[workerCount], sizeof(addresses[workerCount]), 
                         current->workerAddress, _TRUNCATE);
                ports[workerCount] = current->workerPeerPort;
                workerCount++;
            }
            current = current->next;
        } while (current != list->head && workerCount < MAX_WORKERS);
    }

    int successCount = 0;
    current = list->head;
    
    if (current != NULL) {
        do {
            if (current->isConnected && current->workerSocket != INVALID_SOCKET) {
                int result = SendWorkerRegistryEntries(current->workerSocket, list);
                if (result >= 0) {
                    successCount++;
                } else if (result == -2 || result == -3) {
                    PrintInfo("Connection to worker %d lost (error: %d), marking worker as disconnected", current->workerId, result);
                    current->isConnected = 0;
                    SafeCloseSocket(&current->workerSocket);
                } else {
                    PrintWarning("Failed to send registry update to worker %d (error: %d)", current->workerId, result);
                }
            } else {
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);

    PrintInfo("Broadcasted worker registry update to %d/%d workers", successCount, list->count);
    return successCount;
}

int SetWorkerReady(WorkerList* list, int workerId) {
    if (!list) {
        PrintError("Invalid worker list");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* current = list->head;
    int result = -1;

    if (current != NULL) {
        do {
            if (current->workerId == workerId) {
                if (!current->isReady) {
                    PrintInfo("Marking worker %d as ready", workerId);
                    current->isReady = 1;
                    result = 1;
                } else {
                    result = 0;
                }
                break;
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);
    return result;
}

int SetWorkerNotReady(WorkerList* list, int workerId) {
    if (!list) {
        PrintError("Invalid worker list");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* current = list->head;
    int result = -1;

    if (current != NULL) {
        do {
            if (current->workerId == workerId) {
                if (current->isReady) {
                    PrintInfo("Marking worker %d as not ready", workerId);
                    current->isReady = 0;
                    result = 1;
                } else {
                    result = 0;
                }
                break;
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);
    return result;
}

int UpdateWorkerPeerPort(WorkerList* list, int workerId, int peerPort) {
    if (!list) {
        PrintError("Invalid worker list");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* current = list->head;
    int result = -1;

    if (current != NULL) {
        do {
            if (current->workerId == workerId) {
                current->workerPeerPort = peerPort;
                PrintDebug("Updated peer port for worker %d to %d", workerId, peerPort);
                result = 0;
                break;
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);
    return result;
}

int SendWorkerRegistryToSingleWorker(WorkerList* list, int targetWorkerId) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'SendWorkerRegistryToSingleWorker'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (list->count == 0) {
        LeaveCriticalSection(&list->lock);
        PrintInfo("No workers to send registry update to.");
        return 0;
    }

    int workerIds[MAX_WORKERS];
    char addresses[MAX_WORKERS][256];
    int ports[MAX_WORKERS];
    int workerCount = 0;
    
    
    WorkerNode* current = list->head;
    WorkerNode* targetWorker = NULL;
    
    if (current != NULL) {
        do {
            if (workerCount < MAX_WORKERS) {
                workerIds[workerCount] = current->workerId;
                strncpy_s(addresses[workerCount], sizeof(addresses[workerCount]), 
                         current->workerAddress, _TRUNCATE);
                ports[workerCount] = current->workerPeerPort;
                workerCount++;
            }
            
            if (current->workerId == targetWorkerId) {
                targetWorker = current;
            }
            
            current = current->next;
        } while (current != list->head && workerCount < MAX_WORKERS);
    }

    if (targetWorker == NULL) {
        LeaveCriticalSection(&list->lock);
        PrintWarning("Target worker %d not found in worker list", targetWorkerId);
        return -1;
    }
    
    if (targetWorker->isConnected && targetWorker->workerSocket != INVALID_SOCKET) {
        PrintInfo("Sending registry update directly to worker %d", targetWorkerId);
        int result = SendWorkerRegistryEntries(targetWorker->workerSocket, list);
        if (result >= 0) {
            PrintInfo("Successfully sent registry update to worker %d", targetWorkerId);
            LeaveCriticalSection(&list->lock);
            return 1;
        } else if (result == -2 || result == -3) {
            PrintInfo("Connection to worker %d lost (error: %d), marking worker as disconnected", targetWorkerId, result);
            targetWorker->isConnected = 0;
            SafeCloseSocket(&targetWorker->workerSocket);
        } else {
            PrintWarning("Failed to send registry update to worker %d (error: %d)", targetWorkerId, result);
        }
    } else {
    }

    LeaveCriticalSection(&list->lock);
    return 0;
}

int GetWorkerById(WorkerList* list, int workerId, WorkerNode* worker) {
    if (!list || !worker) {
        PrintError("Invalid parameters provided to 'GetWorkerById'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* current = list->head;
    int result = 0;

    if (current != NULL) {
        do {
            if (current->workerId == workerId) {
                worker->workerId = current->workerId;
                worker->workerSocket = current->workerSocket;
                worker->isConnected = current->isConnected;
                worker->isReady = current->isReady;
                worker->workerPeerPort = current->workerPeerPort;
                strncpy_s(worker->workerAddress, sizeof(worker->workerAddress), 
                         current->workerAddress, _TRUNCATE);
                
                result = 1;
                break;
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);
    return result;
}

int SendWorkerRegistryEntries(SOCKET socket, WorkerList* list) {
    if (socket == INVALID_SOCKET || list == NULL) {
        PrintError("Invalid parameters provided to 'SendWorkerRegistryEntries'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (SendWorkerRegistryStart(socket, list->count) <= 0) {
        LeaveCriticalSection(&list->lock);
        return -1;
    }

    WorkerNode* current = list->head;
    int sentCount = 0;
    
    if (current != NULL) {
        do {
            if (SendWorkerEntry(socket, current->workerId, current->workerAddress, current->workerPeerPort, 0) <= 0) {
                LeaveCriticalSection(&list->lock);
                return -1;
            }
            sentCount++;
            current = current->next;
        } while (current != list->head);
    }

    if (SendWorkerRegistryEnd(socket) <= 0) {
        LeaveCriticalSection(&list->lock);
        return -1;
    }

    LeaveCriticalSection(&list->lock);
    return sentCount;
}

int BroadcastNewWorkerJoined(WorkerList* list, int newWorkerId) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'BroadcastNewWorkerJoined'.");
        return -1;
    }

    EnterCriticalSection(&list->lock);

    if (list->count <= 1) {
        LeaveCriticalSection(&list->lock);
        PrintInfo("Only new worker in system, no existing workers to notify.");
        return 0;
    }

    WorkerNode* newWorker = NULL;
    WorkerNode* sourceWorker = NULL;
    WorkerNode* current = list->head;
    
    if (current != NULL) {
        do {
            if (current->workerId == newWorkerId) {
                newWorker = current;
            } else if (sourceWorker == NULL && current->isConnected && current->isReady) {
                sourceWorker = current;
            }
            current = current->next;
        } while (current != list->head && (newWorker == NULL || sourceWorker == NULL));
    }

    if (newWorker == NULL) {
        LeaveCriticalSection(&list->lock);
        PrintError("New worker %d not found in worker list", newWorkerId);
        return -1;
    }

    int successCount = 0;
    current = list->head;
    
    if (current != NULL) {
        do {
            if (current->workerId != newWorkerId && current->isConnected && current->workerSocket != INVALID_SOCKET) {
                if (SendWorkerRegistryStart(current->workerSocket, list->count) <= 0) {
                    PrintWarning("Failed to send registry start to worker %d", current->workerId);
                    current = current->next;
                    continue;
                }
                
                uint8_t shouldExport = (current == sourceWorker) ? 1 : 0;
                if (SendWorkerEntry(current->workerSocket, newWorker->workerId, newWorker->workerAddress, newWorker->workerPeerPort, shouldExport) <= 0) {
                    PrintWarning("Failed to send new worker entry to worker %d", current->workerId);
                } else {
                    if (shouldExport) {
                        PrintInfo("Told worker %d to export data to new worker %d", current->workerId, newWorkerId);
                    }
                    successCount++;
                }
                
                if (SendWorkerRegistryEnd(current->workerSocket) <= 0) {
                    PrintWarning("Failed to send registry end to worker %d", current->workerId);
                }
            }
            current = current->next;
        } while (current != list->head);
    }

    LeaveCriticalSection(&list->lock);
    PrintInfo("Notified %d existing workers about new worker %d", successCount, newWorkerId);
    return successCount;
}


