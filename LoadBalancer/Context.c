﻿#include "Context.h"

int ContextInitialize(Context* context) {
    if (context == NULL) {
        PrintError("Invalid context provided to 'ContextInitialize'.");

        return -1;
    }

    InitializeCriticalSection(&context->lock);

    context->finishSignal = CreateSemaphore(0, 0, MAX_CLIENTS + MAX_WORKERS + THREAD_COUNT, NULL);
    if (context->finishSignal == NULL) {
        PrintCritical("Failed to create a semaphore for the finish signal.");

        return GetLastError();
    }

    context->finishFlag = 0;  

    context->clientListenSocket = INVALID_SOCKET;

    context->workerListenSocket = INVALID_SOCKET;

    context->clientConnectionResultingAddress = NULL;

    context->workerConnectionResultingAddress = NULL;

    context->workerList = (WorkerList*)malloc(sizeof(WorkerList));
    if (context->workerList == NULL) {
        PrintCritical("Failed to allocate memory for WorkerList.");

        return -1;
    }
    if (InitializeWorkerList(context->workerList) != 0) {
        PrintCritical("Failed to initialize WorkerList.");

        return -1;
    }

    context->clientThreadPool = (ClientThreadPool*)malloc(sizeof(ClientThreadPool));
    if (context->clientThreadPool == NULL) {
        PrintCritical("Failed to allocate memory for ClientThreadPool.");

        return -1;
    }
    if (InitializeClientThreadPool(context->clientThreadPool) != 0) {
        PrintCritical("Failed to initialize ClientThreadPool.");

        return -1;
    }

    context->workerThreadPool = (WorkerThreadPool*)malloc(sizeof(WorkerThreadPool));
    if (context->workerThreadPool == NULL) {
        PrintCritical("Failed to allocate memory for WorkerThreadPool.");

        return -1;
    }
    if (InitializeWorkerThreadPool(context->workerThreadPool) != 0) {
        PrintCritical("Failed to initialize WorkerThreadPool.");

        return -1;
    }

    context->clientRequestQueue = (ClientRequestQueue*)malloc(sizeof(ClientRequestQueue));
    if (context->clientRequestQueue == NULL) {
        PrintCritical("Failed to allocate memory for ClientRequestQueue.");

        return -1;
    }
    if (InitializeClientRequestQueue(context->clientRequestQueue) != 0) {
        PrintCritical("Failed to initialize ClientRequestQueue.");

        return -1;
    }

    context->workerResponseQueue = (WorkerResponseQueue*)malloc(sizeof(WorkerResponseQueue));
    if (context->workerResponseQueue == NULL) {
        PrintCritical("Failed to allocate memory for WorkerResponseQueue.");

        return -1;
    }
    if (InitializeWorkerResponseQueue(context->workerResponseQueue) != 0) {
        PrintCritical("Failed to initialize WorkerResponseQueue.");

        return -1;
    }

    return 0;
}

int ContextDestroy(Context* context) {
    if (context == NULL) {
        PrintError("Invalid context provided to 'ContextDestroy'.");

        return -1;
    }

    EnterCriticalSection(&context->lock);

    if (context->workerResponseQueue != NULL) {
        if (DestroyWorkerResponseQueue(context->workerResponseQueue) != 0) {
            PrintWarning("Failed to destroy WorkerResponseQueue.");
        }
        free(context->workerResponseQueue);
        context->workerResponseQueue = NULL;
    }

    if (context->workerThreadPool != NULL) {
        if (DestroyWorkerThreadPool(context->workerThreadPool) != 0) {
            PrintWarning("Failed to destroy WorkerThreadPool.");
        }
        free(context->workerThreadPool);
        context->workerThreadPool = NULL;
    }

    if (context->workerList != NULL) {
        if (DestroyWorkerList(context->workerList) != 0) {
            PrintWarning("Failed to destroy WorkerList.");
        }
        free(context->workerList);
        context->workerList = NULL;
    }

    if (context->clientRequestQueue != NULL) {
        if (DestroyClientRequestQueue(context->clientRequestQueue) != 0) {
            PrintWarning("Failed to destroy ClientRequestQueue.");
        }
        free(context->clientRequestQueue);
        context->clientRequestQueue = NULL;
    }

    if (context->clientThreadPool != NULL) {
        if (DestroyClientThreadPool(context->clientThreadPool) != 0) {
            PrintWarning("Failed to destroy ClientThreadPool.");
        }
        free(context->clientThreadPool);
        context->clientThreadPool = NULL;
    }

    context->workerConnectionResultingAddress = NULL;

    context->clientConnectionResultingAddress = NULL;

    context->workerListenSocket = INVALID_SOCKET;

    context->clientListenSocket = INVALID_SOCKET;

    context->finishFlag = 0;  

    if (context->finishSignal != NULL) {
        CloseHandle(context->finishSignal);
        context->finishSignal = NULL;
    }

    LeaveCriticalSection(&context->lock);
    DeleteCriticalSection(&context->lock);

    return 0;
}

int SetFinishSignal(Context* context) {
    EnterCriticalSection(&context->lock);

    if (context->finishFlag != true) {
        context->finishFlag = true;
        ReleaseSemaphore(context->finishSignal, THREAD_COUNT, NULL);
    }

    LeaveCriticalSection(&context->lock);

    return 0;
}

int GetFinishFlag(Context* context) {
    if (context == NULL) {
        PrintError("Invalid context provided to 'GetFinishFlag'.");

        return 0;  
    }

    EnterCriticalSection(&context->lock);

    int flag = context->finishFlag;

    LeaveCriticalSection(&context->lock);

    return flag;
}

int GetWorkerAddress(SOCKET socket, char* addressBuffer, size_t bufferSize) {
    if (socket == INVALID_SOCKET || addressBuffer == NULL || bufferSize == 0) {
        return -1;
    }

    struct sockaddr_in addr;
    int addrLen = sizeof(addr);
    
    if (getpeername(socket, (struct sockaddr*)&addr, &addrLen) == SOCKET_ERROR) {
        PrintWarning("Failed to get socket address: %d", WSAGetLastError());
        strcpy_s(addressBuffer, bufferSize, "127.0.0.1");
        return 0;
    }
    
    if (inet_ntop(AF_INET, &addr.sin_addr, addressBuffer, (int)bufferSize) == NULL) {
        PrintWarning("Failed to convert address to string");
        strcpy_s(addressBuffer, bufferSize, "127.0.0.1");
        return 0;
    }
    
    return 0;
}


