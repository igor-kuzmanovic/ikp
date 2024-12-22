#include "ClientThreadPool.h"


void InitializeClientThreadPool(ClientThreadPool* pool) {
    InitializeCriticalSection(&pool->lock);

    pool->semaphore = CreateSemaphore(NULL, CLIENT_THREAD_POOL_SIZE, CLIENT_THREAD_POOL_SIZE, NULL);

    for (int i = 0; i < CLIENT_THREAD_POOL_SIZE; i++) {
        pool->threads[i] = NULL;
        pool->clientSockets[i] = INVALID_SOCKET;
        pool->isAvailable[i] = true;
    }

    pool->count = 0;

    PrintDebug("Client thread pool initialized with size: %d.", CLIENT_THREAD_POOL_SIZE);
}

void DestroyClientThreadPool(ClientThreadPool* pool) {
    PrintDebug("Destroying client thread pool, waiting for threads to finish.");
    WaitForMultipleObjects(CLIENT_THREAD_POOL_SIZE, pool->threads, TRUE, INFINITE);

    pool->count = 0;

    for (int i = 0; i < CLIENT_THREAD_POOL_SIZE; i++) {
        pool->isAvailable[i] = true;

        if (pool->clientSockets[i] != INVALID_SOCKET) {
            closesocket(pool->clientSockets[i]);
            pool->clientSockets[i] = INVALID_SOCKET;
        }

        if (pool->threads[i] != NULL) {
            CloseHandle(pool->threads[i]);
            pool->threads[i] = NULL;
        }
    }

    if (pool->semaphore != NULL) {
        CloseHandle(pool->semaphore);
        pool->semaphore = NULL;
    }

    DeleteCriticalSection(&pool->lock);

    PrintDebug("Client thread pool destroyed");
}

int AssignClientDataReceiverThread(ClientThreadPool* pool, SOCKET clientSocket, Context *ctx) {
    WaitForSingleObject(pool->semaphore, INFINITE);
    EnterCriticalSection(&pool->lock);

    for (int i = 0; i < CLIENT_THREAD_POOL_SIZE; i++) {
        if (pool->isAvailable[i]) {
            pool->isAvailable[i] = false;
            pool->clientSockets[i] = clientSocket;

            ClientDataReceiverThreadData* data = (ClientDataReceiverThreadData*)malloc(sizeof(ClientDataReceiverThreadData));
            if (!data) {
                LeaveCriticalSection(&pool->lock);
                ReleaseSemaphore(pool->semaphore, 1, NULL);

                return -1;
            }

            data->ctx = ctx;
            data->clientSocket = clientSocket;
            data->threadIndex = i;

            pool->threads[i] = CreateThread(NULL, 0, ClientDataReceiverThread, data, 0, NULL);
            if (pool->threads[i] == NULL) {
                free(data);

                pool->isAvailable[i] = true;
                pool->clientSockets[i] = INVALID_SOCKET;

                LeaveCriticalSection(&pool->lock);
                ReleaseSemaphore(pool->semaphore, 1, NULL);

                return -1;
            }

            pool->count++;

            LeaveCriticalSection(&pool->lock);

            PrintDebug("Client data receiver thread assigned, thread index: %d.", i);
            PrintDebug("Client thread pool: %d/%d.", pool->count, CLIENT_THREAD_POOL_SIZE);

            return i;
        }
    }

    LeaveCriticalSection(&pool->lock);

    return -1;
}

void ReturnClientDataReceiverThread(ClientThreadPool* pool, int threadIndex, ClientDataReceiverThreadData* data) {
    EnterCriticalSection(&pool->lock);

    CloseHandle(pool->threads[threadIndex]);
    pool->threads[threadIndex] = NULL;
    pool->isAvailable[threadIndex] = true;
    pool->clientSockets[threadIndex] = INVALID_SOCKET;
    free(data);

    pool->count--;

    LeaveCriticalSection(&pool->lock);
    ReleaseSemaphore(pool->semaphore, 1, NULL);

    PrintDebug("Client data receiver thread returned, thread index: %d.", threadIndex);
    PrintDebug("Client thread pool: %d/%d.", pool->count, CLIENT_THREAD_POOL_SIZE);
}