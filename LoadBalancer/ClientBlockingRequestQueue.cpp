#include "ClientBlockingRequestQueue.h"

void InitializeClientBlockingRequestQueue(ClientBlockingRequestQueue* queue) {
    PrintDebug("Initializing client blocking request queue.");

    InitializeCriticalSection(&queue->lock);
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->notEmpty = CreateSemaphore(NULL, 0, CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY, NULL);
    queue->notFull = CreateSemaphore(NULL, CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY, CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY, NULL);

    PrintDebug("Client blocking request queue initialized with a capacity of %d.", CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY);
}

void DestroyClientBlockingRequestQueue(ClientBlockingRequestQueue* queue) {
    PrintDebug("Destroying client blocking request queue.");

    CloseHandle(queue->notFull);
    CloseHandle(queue->notEmpty);
    queue->count = 0;
    queue->tail = 0;
    queue->head = 0;
    DeleteCriticalSection(&queue->lock);

    PrintDebug("Client blocking request queue destroyed.");
}

int PutClientBlockingRequestQueue(ClientBlockingRequestQueue* queue, SOCKET clientSocket, const char* data) {
    PrintDebug("Waiting for a free slot in the client blocking request queue with a %d ms timeout.", CLIENT_BLOCKING_REQUEST_QUEUE_PUT_TIMEOUT);
    DWORD waitResult = WaitForSingleObject(queue->notFull, CLIENT_BLOCKING_REQUEST_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        PrintDebug("Client blocking request queue is full, rejecting the request.");

        return -1;
    }

    if (waitResult == WAIT_OBJECT_0) {
        PrintDebug("Entering the client blocking request queue critical section.");
        EnterCriticalSection(&queue->lock);

        PrintDebug("Putting a request in the client blocking request queue.");
        queue->queue[queue->tail].clientSocket = clientSocket;
        memcpy(queue->queue[queue->tail].data, data, BUFFER_SIZE);
        queue->tail = (queue->tail + 1) % CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY;
        queue->count++;

        PrintDebug("Leaving the client blocking request queue critical section.");
        LeaveCriticalSection(&queue->lock);

        PrintDebug("Signaling that there is data available in the client blocking request queue.");
        ReleaseSemaphore(queue->notEmpty, 1, NULL);

        PrintDebug("Client blocking request queue: %d/%d", queue->count, CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY);

        return 0;
    }

    PrintError("Failed to put the request in the client blocking request queue.");

    return -1;
}

ClientRequest TakeClientBlockingRequestQueue(ClientBlockingRequestQueue* queue) {
    ClientRequest request;

    PrintDebug("Waiting for data in the client blocking request queue with an infinite timeout.");
    WaitForSingleObject(queue->notEmpty, CLIENT_BLOCKING_REQUEST_QUEUE_TAKE_TIMEOUT);

    PrintDebug("Entering the client blocking request queue critical section.");
    EnterCriticalSection(&queue->lock);

    PrintDebug("Taking a request from the client blocking request queue.");
    request = queue->queue[queue->head];
    queue->head = (queue->head + 1) % CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY;
    queue->count--;

    PrintDebug("Leaving the client blocking request queue critical section.");
    LeaveCriticalSection(&queue->lock);

    PrintDebug("Signaling that there is a free slot in the client blocking request queue.");
    ReleaseSemaphore(queue->notFull, 1, NULL);

    PrintDebug("Client blocking request queue: %d/%d", queue->count, CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY);

    return request;
}
