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

    list->semaphore = CreateSemaphore(NULL, 0, MAX_WORKERS, NULL);
    if (list->semaphore == NULL) {
        PrintCritical("Failed to create a semaphore for the worker list.");

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

    CloseHandle(list->semaphore);

    LeaveCriticalSection(&list->lock);
    DeleteCriticalSection(&list->lock);

    PrintDebug("Worker list destroyed.");

    return 0;
}

int AddWorker(WorkerList* list, SOCKET workerSocket) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'AddWorker'.");

        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid socket provided to 'AddWorker'.");

        return -1;
    }

    PrintDebug("Entering the worker list critical section.");
    EnterCriticalSection(&list->lock);

    if (list->count >= MAX_WORKERS) {
        PrintError("Worker list is at maximum capacity (%d).", MAX_WORKERS);

        PrintDebug("Leaving the worker list critical section.");
        LeaveCriticalSection(&list->lock);

        return -1;
    }

    PrintDebug("Allocating memory for a new worker node.");
    WorkerNode* newNode = (WorkerNode*)malloc(sizeof(WorkerNode));
    if (!newNode) {
        PrintError("Failed to allocate memory for a new worker node.");

        PrintDebug("Leaving the worker list critical section.");
        LeaveCriticalSection(&list->lock);

        return -1;
    }

    newNode->socket = workerSocket;

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

    PrintDebug("Signaling that a worker is available.");
    ReleaseSemaphore(list->semaphore, 1, NULL);

    PrintDebug("Leaving the worker list critical section.");
    LeaveCriticalSection(&list->lock);

    PrintDebug("Worker list: %d/%d", list->count, MAX_WORKERS);

    return 0;
}

int RemoveWorker(WorkerList* list, SOCKET workerSocket) {
    if (list == NULL) {
        PrintError("Invalid worker list provided to 'RemoveWorker'.");

        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid socket provided to 'RemoveWorker'.");
    
        return -1;
    }

    PrintDebug("Entering the worker list critical section.");
    EnterCriticalSection(&list->lock);

    WorkerNode* temp = list->head;
    bool isFound = false;

    if (temp) {
        do {
            if (temp->socket == workerSocket) {
                PrintDebug("Found the worker to remove: socket %d.", workerSocket);
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

                PrintDebug("Worker removed: socket %d.", workerSocket);

                break;
            }

            temp = temp->next;
        } while (temp != list->head);
    }

    PrintDebug("Leaving the worker list critical section.");
    LeaveCriticalSection(&list->lock);

    if (!isFound) {
        PrintWarning("Worker with socket %d not found.", workerSocket);

        return -1;
    }

    WaitForSingleObject(list->semaphore, 0);

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

    PrintDebug("Waiting for a worker to be available with a %d ms timeout.", WORKER_LIST_GET_TIMEOUT);
    DWORD waitResult = WaitForSingleObject(list->semaphore, WORKER_LIST_GET_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        PrintDebug("No worker is available.");

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to get the next worker.");

        return -1;
    }

    PrintDebug("Entering the worker list critical section.");
    EnterCriticalSection(&list->lock);

    if (list->count == 0 || list->current == NULL) {
        PrintError("Worker list is empty after semaphore signal.");

        PrintDebug("Leaving the worker list critical section.");
        LeaveCriticalSection(&list->lock);

        return -1;
    }

    PrintDebug("Getting the next worker.");
    WorkerNode* currentNode = list->current;
    list->current = list->current->next;
    worker->socket = currentNode->socket;

    PrintDebug("Leaving the worker list critical section.");
    LeaveCriticalSection(&list->lock);

    PrintDebug("Worker list: %d/%d", list->count, MAX_WORKERS);

    // TODO Remove this later
    ReleaseSemaphore(list->semaphore, 1, NULL);

    return 1;
}
