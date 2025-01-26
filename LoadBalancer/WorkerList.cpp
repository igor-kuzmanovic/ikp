#include "WorkerList.h"

int InitializeWorkerList(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'InitializeWorkerList'.");

        return -1;
    }

    PrintDebug("Initializing worker list.");

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

    PrintDebug("Worker list initialized with a capacity of %d.", MAX_WORKERS);

    return 0;
}

int DestroyWorkerList(WorkerList* list) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'DestroyWorkerList'.");

        return -1;
    }

    PrintDebug("Destroying worker list.");

    EnterCriticalSection(&list->lock);

    WorkerNode* temp = list->head;
    if (temp) {
        do {
            WorkerNode* next = temp->next;
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

    PrintDebug("Worker list destroyed.");

    return 0;
}

int AddWorker(WorkerList* list, const SOCKET workerSocket, const int workerId) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'AddWorker'.");

        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid socket provided to 'AddWorker'.");

        return -1;
    }

    // Wait for the signal that there is a free slot in the list
    DWORD waitResult = WaitForSingleObject(list->notFull, WORKER_LIST_ADD_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // List is full

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

    PrintDebug("Allocating memory for a new worker node.");
    WorkerNode* newNode = (WorkerNode*)malloc(sizeof(WorkerNode));
    if (!newNode) {
        PrintError("Failed to allocate memory for a new worker node.");

        LeaveCriticalSection(&list->lock);

        return -1;
    }

    newNode->workerId = workerId;
    newNode->workerSocket = workerSocket;

    if (list->head == NULL) {
        PrintDebug("Adding the first worker to the list.");
        list->head = newNode;
        list->current = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        PrintDebug("Adding a worker to the end of the list.");
        WorkerNode* tail = list->head->prev;
        tail->next = newNode;
        newNode->prev = tail;
        newNode->next = list->head;
        list->head->prev = newNode;
    }
    list->count++;

    LeaveCriticalSection(&list->lock);

    PrintDebug("Signaling that there is a worker available in the worker list.");
    ReleaseSemaphore(list->notEmpty, 1, NULL);

    PrintDebug("Worker list: %d/%d", list->count, MAX_WORKERS);

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

    // Wait for the signal that there is a worker in the list
    DWORD waitResult = WaitForSingleObject(list->notEmpty, WORKER_LIST_REMOVE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // List is empty

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to remove the worker from the worker list.");

        return -1;
    }

    EnterCriticalSection(&list->lock);

    WorkerNode* temp = list->head;
    bool isFound = false;

    if (temp) {
        do {
            if (temp->workerId == workerId) {
                PrintDebug("Found the worker to remove: id %d, socket %d.", workerId, temp->workerSocket);
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

                PrintDebug("Worker removed: id %d.", workerId);

                break;
            }

            temp = temp->next;
        } while (temp != list->head);
    }

    LeaveCriticalSection(&list->lock);

    PrintDebug("Signaling that there is a free slot in the worker list.");
    ReleaseSemaphore(list->notFull, 1, NULL);

    if (!isFound) {
        PrintWarning("Worker with id %d not found.", workerId);

        return -1;
    }

    PrintDebug("Worker list: %d/%d", list->count, MAX_WORKERS);

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

    PrintDebug("Getting the next worker.");
    WorkerNode* currentNode = list->current;
    list->current = list->current->next;
    worker->workerId = currentNode->workerId;
    worker->workerSocket = currentNode->workerSocket;

    LeaveCriticalSection(&list->lock);

    PrintDebug("Worker list: %d/%d", list->count, MAX_WORKERS);

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
        PrintDebug("Worker list is empty.");

        LeaveCriticalSection(&list->lock);

        return workerId;
    }

    if (*iterator == NULL) {
        // If current is NULL, start from the head of the list
        *iterator = list->head;
        workerId = (*iterator)->workerId;
    } else if ((*iterator)->next == list->head) {
        // If we have looped back to the head node, stop and return NULL
        *iterator = NULL;
        workerId = 0;
    } else {
        // Move to the next worker in the list
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
