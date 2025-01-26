#include "WorkerResponseQueue.h"

int InitializeWorkerResponseQueue(WorkerResponseQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'InitializeWorkerResponseQueue'.");

        return -1;
    }

    PrintDebug("Initializing worker response queue.");

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

    PrintDebug("Worker response queue initialized with a capacity of %d.", WORKER_RESPONSE_QUEUE_CAPACITY);

    return 0;
}

int DestroyWorkerResponseQueue(WorkerResponseQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid worker response queue provided to 'DestroyWorkerResponseQueue'.");

        return -1;
    }

    PrintDebug("Destroying worker response queue.");

    EnterCriticalSection(&queue->lock);

    CloseHandle(queue->notFull);
    CloseHandle(queue->notEmpty);

    queue->count = 0;
    queue->tail = 0;
    queue->head = 0;

    LeaveCriticalSection(&queue->lock);
    DeleteCriticalSection(&queue->lock);

    PrintDebug("Worker response queue destroyed.");

    return 0;
}

int PutWorkerResponseQueue(WorkerResponseQueue* queue, SOCKET workerSocket, const char* data, int length) {
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

    // Wait for the signal that there is a free slot in the queue
    DWORD waitResult = WaitForSingleObject(queue->notFull, WORKER_RESPONSE_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // Queue is full

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

    PrintDebug("Putting a response in the worker response queue.");
    queue->queue[queue->tail].workerSocket = workerSocket;
    memcpy(queue->queue[queue->tail].data.buffer, data, length);

    queue->tail = (queue->tail + 1) % WORKER_RESPONSE_QUEUE_CAPACITY;
    queue->count++;

    LeaveCriticalSection(&queue->lock);

    PrintDebug("Signaling that there is data available in the worker response queue.");
    ReleaseSemaphore(queue->notEmpty, 1, NULL);

    PrintDebug("Worker response queue: %d/%d", queue->count, WORKER_RESPONSE_QUEUE_CAPACITY);

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

    // Wait for the signal that there is data in the queue
    DWORD waitResult = WaitForSingleObject(queue->notEmpty, WORKER_RESPONSE_QUEUE_TAKE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // Queue is empty

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

    PrintDebug("Taking a response from the worker response queue.");
    response->workerSocket = queue->queue[queue->head].workerSocket;
    memcpy(response->data.buffer, queue->queue[queue->head].data.buffer, BUFFER_SIZE);

    queue->head = (queue->head + 1) % WORKER_RESPONSE_QUEUE_CAPACITY;
    queue->count--;

    LeaveCriticalSection(&queue->lock);

    PrintDebug("Signaling that there is a free slot in the worker response queue.");
    ReleaseSemaphore(queue->notFull, 1, NULL);

    PrintDebug("Worker response queue: %d/%d", queue->count, WORKER_RESPONSE_QUEUE_CAPACITY);

    return 1;
}
