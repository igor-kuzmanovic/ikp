#include "WorkerResponseQueue.h"

int InitializeWorkerResponseQueue(WorkerResponseQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'InitializeWorkerResponseQueue'.");

        return -1;
    }

    InitializeCriticalSection(&queue->lock);

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    queue->notEmpty = CreateSemaphore(NULL, 0, WORKER_RESPONSE_QUEUE_CAPACITY, NULL);
    if (queue->notEmpty == NULL) {
        PrintCritical("Failed to create a 'notEmpty' semaphore for the worker response queue.");

        return GetLastError();
    }

    queue->notFull = CreateSemaphore(NULL, WORKER_RESPONSE_QUEUE_CAPACITY, WORKER_RESPONSE_QUEUE_CAPACITY, NULL);
    if (queue->notFull == NULL) {
        PrintCritical("Failed to create a 'notFull' semaphore for the worker response queue.");

        return GetLastError();
    }

    return 0;
}

int DestroyWorkerResponseQueue(WorkerResponseQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'DestroyWorkerResponseQueue'.");

        return -1;
    }

    EnterCriticalSection(&queue->lock);

    CloseHandle(queue->notFull);
    CloseHandle(queue->notEmpty);

    queue->count = 0;
    queue->tail = 0;
    queue->head = 0;

    LeaveCriticalSection(&queue->lock);
    DeleteCriticalSection(&queue->lock);

    return 0;
}

int PutWorkerResponseQueue(WorkerResponseQueue* queue, const SOCKET workerSocket, const char* data, const int length, MessageType messageType, const int workerId, const int clientId) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'PutWorkerResponseQueue'.");

        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid worker socket provided to 'PutWorkerResponseQueue'.");

        return -1;
    }

    if (data == NULL) {
        PrintError("Invalid data provided to 'PutWorkerResponseQueue'.");

        return -1;
    }

    DWORD waitResult = WaitForSingleObject(queue->notFull, WORKER_RESPONSE_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to put the response in the worker response queue.");

        return -1;
    }

    EnterCriticalSection(&queue->lock);

    if (queue->count >= WORKER_RESPONSE_QUEUE_CAPACITY) {
        PrintError("Worker response queue is full (%d) after semaphore signal.", WORKER_RESPONSE_QUEUE_CAPACITY);

        LeaveCriticalSection(&queue->lock);

        return 0;
    }

    queue->queue[queue->tail].workerSocket = workerSocket;
    queue->queue[queue->tail].workerId = workerId;
    queue->queue[queue->tail].clientId = clientId;
    memcpy(queue->queue[queue->tail].data, data, length);
    queue->queue[queue->tail].dataSize = length;
    queue->queue[queue->tail].messageType = messageType;

    queue->tail = (queue->tail + 1) % WORKER_RESPONSE_QUEUE_CAPACITY;
    queue->count++;

    LeaveCriticalSection(&queue->lock);

    ReleaseSemaphore(queue->notEmpty, 1, NULL);

    return 1;
}

int TakeWorkerResponseQueue(WorkerResponseQueue* queue, WorkerResponse* response) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'TakeWorkerResponseQueue'.");

        return -1;
    }

    if (response == NULL) {
        PrintError("Invalid worker response provided to 'TakeWorkerResponseQueue'.");

        return -1;
    }

    DWORD waitResult = WaitForSingleObject(queue->notEmpty, WORKER_RESPONSE_QUEUE_TAKE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to take the response from the worker response queue.");

        return -1;
    }

    EnterCriticalSection(&queue->lock);

    if (queue->count <= 0) {
        PrintError("Worker response queue is empty after semaphore signal.");

        LeaveCriticalSection(&queue->lock);

        return 0;
    }

    response->workerSocket = queue->queue[queue->head].workerSocket;
    response->workerId = queue->queue[queue->head].workerId;
    response->clientId = queue->queue[queue->head].clientId;
    memcpy(response->data, queue->queue[queue->head].data, queue->queue[queue->head].dataSize);
    response->dataSize = queue->queue[queue->head].dataSize;
    response->messageType = queue->queue[queue->head].messageType;

    queue->head = (queue->head + 1) % WORKER_RESPONSE_QUEUE_CAPACITY;
    queue->count--;

    LeaveCriticalSection(&queue->lock);

    ReleaseSemaphore(queue->notFull, 1, NULL);

    return 1;
}


