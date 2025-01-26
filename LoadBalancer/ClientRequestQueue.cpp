#include "ClientRequestQueue.h"

int InitializeClientRequestQueue(ClientRequestQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'InitializeClientRequestQueue'.");

        return -1;
    }

    PrintDebug("Initializing client request queue.");

    InitializeCriticalSection(&queue->lock);

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;

    queue->notEmpty = CreateSemaphore(NULL, 0, CLIENT_REQUEST_QUEUE_CAPACITY, NULL);
    if (queue->notEmpty == NULL) {
        PrintCritical("Failed to create a 'notEmpty' semaphore for the client request queue.");

        return GetLastError();
    }

    queue->notFull = CreateSemaphore(NULL, CLIENT_REQUEST_QUEUE_CAPACITY, CLIENT_REQUEST_QUEUE_CAPACITY, NULL);
    if (queue->notFull == NULL) {
        PrintCritical("Failed to create a 'notFull' semaphore for the client request queue.");

        return GetLastError();
    }

    PrintDebug("Client request queue initialized with a capacity of %d.", CLIENT_REQUEST_QUEUE_CAPACITY);

    return 0;
}

int DestroyClientRequestQueue(ClientRequestQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'DestroyClientRequestQueue'.");

        return -1;
    }

    PrintDebug("Destroying client request queue.");

    EnterCriticalSection(&queue->lock);

    CloseHandle(queue->notFull);
    CloseHandle(queue->notEmpty);

    queue->count = 0;
    queue->tail = 0;
    queue->head = 0;

    LeaveCriticalSection(&queue->lock);
    DeleteCriticalSection(&queue->lock);

    PrintDebug("Client request queue destroyed.");

    return 0;
}

int PutClientRequestQueue(ClientRequestQueue* queue, SOCKET clientSocket, const char* data, int length, const int clientId) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'PutClientRequestQueue'.");

        return -1;
    }

    if (clientSocket == INVALID_SOCKET) {
        PrintError("Invalid client socket provided to 'PutClientRequestQueue'.");

        return -1;
    }

    if (data == NULL) {
        PrintError("Invalid data provided to 'PutClientRequestQueue'.");

        return -1;
    }

    // Wait for the signal that there is a free slot in the queue
    DWORD waitResult = WaitForSingleObject(queue->notFull, CLIENT_REQUEST_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // Queue is full

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to put the request in the client request queue.");

        return -1;
    }

    EnterCriticalSection(&queue->lock);

    if (queue->count >= CLIENT_REQUEST_QUEUE_CAPACITY) {
        PrintError("Client request queue is full (%d) after semaphore signal.", CLIENT_REQUEST_QUEUE_CAPACITY);

        LeaveCriticalSection(&queue->lock);

        return 0;
    }

    PrintDebug("Putting a request in the client request queue.");
    queue->queue[queue->tail].clientSocket = clientSocket;
    queue->queue[queue->tail].clientId = clientId;
    memcpy(queue->queue[queue->tail].data.buffer, data, length);

    queue->tail = (queue->tail + 1) % CLIENT_REQUEST_QUEUE_CAPACITY;
    queue->count++;

    LeaveCriticalSection(&queue->lock);

    PrintDebug("Signaling that there is data available in the client request queue.");
    ReleaseSemaphore(queue->notEmpty, 1, NULL);

    PrintDebug("Client request queue: %d/%d", queue->count, CLIENT_REQUEST_QUEUE_CAPACITY);

    return 1;
}

int TakeClientRequestQueue(ClientRequestQueue* queue, ClientRequest* request) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'TakeClientRequestQueue'.");

        return -1;
    }

    if (request == NULL) {
        PrintError("Invalid client request provided to 'TakeClientRequestQueue'.");

        return -1;
    }

    // Wait for the signal that there is data in the queue
    DWORD waitResult = WaitForSingleObject(queue->notEmpty, CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        // Queue is empty

        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to take the request from the client request queue.");

        return -1;
    }

    EnterCriticalSection(&queue->lock);

    if (queue->count <= 0) {
        PrintError("Client request queue is empty after semaphore signal.");

        LeaveCriticalSection(&queue->lock);

        return 0;
    }

    PrintDebug("Taking a request from the client request queue.");
    request->clientSocket = queue->queue[queue->head].clientSocket;
    request->clientId = queue->queue[queue->head].clientId;
    memcpy(request->data.buffer, queue->queue[queue->head].data.buffer, BUFFER_SIZE);

    queue->head = (queue->head + 1) % CLIENT_REQUEST_QUEUE_CAPACITY;
    queue->count--;

    LeaveCriticalSection(&queue->lock);

    PrintDebug("Signaling that there is a free slot in the client request queue.");
    ReleaseSemaphore(queue->notFull, 1, NULL);

    PrintDebug("Client request queue: %d/%d", queue->count, CLIENT_REQUEST_QUEUE_CAPACITY);

    return 1;
}
