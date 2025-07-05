#include "WorkerThreadPool.h"
#include <stdlib.h>

int InitializeWorkerThreadPool(WorkerThreadPool* pool) {
    if (pool == NULL) {
        PrintError("Invalid worker thread pool provided to 'InitializeWorkerThreadPool'.");

        return -1;
    }

    InitializeCriticalSection(&pool->lock);

    pool->semaphore = CreateSemaphore(NULL, MAX_WORKERS, MAX_WORKERS, NULL);

    for (int i = 0; i < MAX_WORKERS; i++) {
        pool->threads[i] = NULL;
        pool->workerSockets[i] = INVALID_SOCKET;
        pool->isAvailable[i] = 1;
    }

    pool->count = 0;

    return 0;
}

int DestroyWorkerThreadPool(WorkerThreadPool* pool) {
    if (pool == NULL) {
        PrintError("Invalid worker thread pool provided to 'DestroyWorkerThreadPool'.");

        return -1;
    }

    WaitForMultipleObjects(MAX_WORKERS, pool->threads, TRUE, INFINITE);

    pool->count = 0;

    for (int i = 0; i < MAX_WORKERS; i++) {
        pool->isAvailable[i] = 1;

        if (pool->workerSockets[i] != INVALID_SOCKET) {
            SafeCloseSocket(&pool->workerSockets[i]);
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

    return 0;
}

int AssignWorkerDataReceiverThread(WorkerThreadPool* pool, const SOCKET workerSocket,  Context* context, const int workerId) {
    if (pool == NULL) {
        PrintError("Invalid worker thread pool provided to 'AssignWorkerDataReceiverThread'.");

        return -1;
    }

    if (workerSocket == INVALID_SOCKET) {
        PrintError("Invalid socket provided to 'AssignWorkerDataReceiverThread'.");

        return -1;
    }

    if (context == NULL) {
        PrintError("Invalid context provided to 'AssignWorkerDataReceiverThread'.");

        return -1;
    }

    DWORD waitResult = WaitForSingleObject(pool->semaphore, WORKER_THREAD_POOL_ASSIGN_TIMEOUT);

    if (waitResult != WAIT_OBJECT_0) {

        return -1;
    }

    if (waitResult == WAIT_OBJECT_0) {
        EnterCriticalSection(&pool->lock);

        for (int i = 0; i < MAX_WORKERS; i++) {
            if (pool->isAvailable[i]) {
                pool->isAvailable[i] = 0;
                pool->workerSockets[i] = workerSocket;

                WorkerDataReceiverThreadData* data = (WorkerDataReceiverThreadData*)malloc(sizeof(WorkerDataReceiverThreadData));
                if (!data) {
                    PrintError("Failed to allocate memory for worker data receiver thread data.");

                    LeaveCriticalSection(&pool->lock);
                    ReleaseSemaphore(pool->semaphore, 1, NULL);

                    return -1;
                }

                data->context = context;
                data->workerSocket = workerSocket;
                data->workerId = workerId;
                data->threadIndex = i;

                pool->threads[i] = CreateThread(NULL, 0, WorkerDataReceiverThread, data, 0, NULL);
                if (pool->threads[i] == NULL) {
                    free(data);

                    pool->isAvailable[i] = 1;
                    pool->workerSockets[i] = INVALID_SOCKET;

                    LeaveCriticalSection(&pool->lock);
                    ReleaseSemaphore(pool->semaphore, 1, NULL);

                    return -1;
                }

                pool->count++;

                LeaveCriticalSection(&pool->lock);

                return i;
            }
        }

        LeaveCriticalSection(&pool->lock);
    }

    PrintError("Failed to assign a worker data receiver thread.");

    return -1;
}

int ReturnWorkerDataReceiverThread(WorkerThreadPool* pool, const int threadIndex, WorkerDataReceiverThreadData* data) {
    if (pool == NULL) {
        PrintError("Invalid worker thread pool provided to 'ReturnWorkerDataReceiverThread'.");

        return -1;
    }

    if (threadIndex < 0 || threadIndex >= MAX_WORKERS) {
        PrintError("Invalid thread index provided to 'ReturnWorkerDataReceiverThread'.");

        return -1;
    }

    if (data == NULL) {
        PrintError("Invalid data provided to 'ReturnWorkerDataReceiverThread'.");

        return -1;
    }

    EnterCriticalSection(&pool->lock);

    CloseHandle(pool->threads[threadIndex]);
    pool->threads[threadIndex] = NULL;
    pool->isAvailable[threadIndex] = 1;
    pool->workerSockets[threadIndex] = INVALID_SOCKET;
    free(data);

    pool->count--;

    LeaveCriticalSection(&pool->lock);
    ReleaseSemaphore(pool->semaphore, 1, NULL);

    return 0;
}


