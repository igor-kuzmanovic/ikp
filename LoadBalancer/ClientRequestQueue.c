#include "ClientRequestQueue.h"

int InitializeClientRequestQueue(ClientRequestQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'InitializeClientRequestQueue'.");

        return -1;
    }

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

    return 0;
}

int DestroyClientRequestQueue(ClientRequestQueue* queue) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'DestroyClientRequestQueue'.");

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

int PutClientRequestQueue(ClientRequestQueue* queue, SOCKET clientSocket, const char* data, int length, MessageType messageType, const int clientId) {
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

    DWORD waitResult = WaitForSingleObject(queue->notFull, CLIENT_REQUEST_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

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

    queue->queue[queue->tail].clientSocket = clientSocket;
    queue->queue[queue->tail].clientId = clientId;
    memcpy(queue->queue[queue->tail].data, data, length);
    queue->queue[queue->tail].dataSize = length;
    queue->queue[queue->tail].messageType = messageType;

    queue->tail = (queue->tail + 1) % CLIENT_REQUEST_QUEUE_CAPACITY;
    queue->count++;

    LeaveCriticalSection(&queue->lock);

    ReleaseSemaphore(queue->notEmpty, 1, NULL);

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

    DWORD waitResult = WaitForSingleObject(queue->notEmpty, CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {

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

    request->clientSocket = queue->queue[queue->head].clientSocket;
    request->clientId = queue->queue[queue->head].clientId;
    memcpy(request->data, queue->queue[queue->head].data, queue->queue[queue->head].dataSize);
    request->dataSize = queue->queue[queue->head].dataSize;
    request->messageType = queue->queue[queue->head].messageType;

    queue->head = (queue->head + 1) % CLIENT_REQUEST_QUEUE_CAPACITY;
    queue->count--;

    LeaveCriticalSection(&queue->lock);

    ReleaseSemaphore(queue->notFull, 1, NULL);

    return 1;
}

int ReturnClientRequestQueue(ClientRequestQueue* queue, ClientRequest* request) {
    if (queue == NULL) {
        PrintError("Invalid client request queue provided to 'ReturnClientRequestQueue'.");
        return -1;
    }

    if (request == NULL) {
        PrintError("Invalid client request provided to 'ReturnClientRequestQueue'.");
        return -1;
    }

    DWORD waitResult = WaitForSingleObject(queue->notFull, CLIENT_REQUEST_QUEUE_PUT_TIMEOUT);

    if (waitResult == WAIT_TIMEOUT) {
        PrintWarning("Failed to return request to queue - queue is full");
        return 0;
    }

    if (waitResult != WAIT_OBJECT_0) {
        PrintError("Failed to return the request to the client request queue.");
        return -1;
    }

    EnterCriticalSection(&queue->lock);

    if (queue->count >= CLIENT_REQUEST_QUEUE_CAPACITY) {
        PrintError("Client request queue is full (%d) after semaphore signal.", CLIENT_REQUEST_QUEUE_CAPACITY);
        LeaveCriticalSection(&queue->lock);
        return 0;
    }

    queue->queue[queue->tail].clientSocket = request->clientSocket;
    queue->queue[queue->tail].clientId = request->clientId;
    memcpy(queue->queue[queue->tail].data, request->data, request->dataSize);
    queue->queue[queue->tail].dataSize = request->dataSize;
    queue->queue[queue->tail].messageType = request->messageType;

    queue->tail = (queue->tail + 1) % CLIENT_REQUEST_QUEUE_CAPACITY;
    queue->count++;

    LeaveCriticalSection(&queue->lock);

    ReleaseSemaphore(queue->notEmpty, 1, NULL);

    return 1;
}


