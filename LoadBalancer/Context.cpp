#include "Context.h"

int ContextInitialize(Context* ctx) {
    if (ctx == NULL) {
        PrintError("Invalid context provided to 'ContextInitialize'.");

        return -1;
    }

    InitializeCriticalSection(&ctx->lock);

    ctx->finishSignal = CreateSemaphore(0, 0, MAX_CLIENTS + MAX_WORKERS + THREAD_COUNT, NULL);
    if (ctx->finishSignal == NULL) {
        PrintCritical("Failed to create a semaphore for the finish signal.");

        return GetLastError();
    }

    ctx->finishFlag = false;

    ctx->clientListenSocket = INVALID_SOCKET;

    ctx->workerListenSocket = INVALID_SOCKET;

    ctx->clientConnectionResultingAddress = NULL;

    ctx->workerConnectionResultingAddress = NULL;

    ctx->workerList = (WorkerList*)malloc(sizeof(WorkerList));
    if (ctx->workerList == NULL) {
        PrintCritical("Failed to allocate memory for WorkerList.");

        return -1;
    }
    InitializeWorkerList(ctx->workerList);

    ctx->clientThreadPool = (ClientThreadPool*)malloc(sizeof(ClientThreadPool));
    if (ctx->clientThreadPool == NULL) {
        PrintCritical("Failed to allocate memory for ClientThreadPool.");

        return -1;
    }
    InitializeClientThreadPool(ctx->clientThreadPool);

    ctx->clientRequestQueue = (ClientRequestQueue*)malloc(sizeof(ClientRequestQueue));
    if (ctx->clientRequestQueue == NULL) {
        PrintCritical("Failed to allocate memory for ClientRequestQueue.");

        return -1;
    }
    InitializeClientRequestQueue(ctx->clientRequestQueue);

    return 0;
}

int ContextDestroy(Context* ctx) {
    if (ctx == NULL) {
        PrintError("Invalid context provided to 'ContextDestroy'.");

        return -1;
    }

    EnterCriticalSection(&ctx->lock);

    DestroyWorkerList(ctx->workerList);
    free(ctx->workerList);
    ctx->workerList = NULL;

    DestroyClientRequestQueue(ctx->clientRequestQueue);
    free(ctx->clientRequestQueue);
    ctx->clientRequestQueue = NULL;

    DestroyClientThreadPool(ctx->clientThreadPool);
    free(ctx->clientThreadPool);
    ctx->clientThreadPool = NULL;

    ctx->workerConnectionResultingAddress = NULL;

    ctx->clientConnectionResultingAddress = NULL;

    ctx->workerListenSocket = INVALID_SOCKET;

    ctx->clientListenSocket = INVALID_SOCKET;

    ctx->finishFlag = false;

    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
        ctx->finishSignal = NULL;
    }

    LeaveCriticalSection(&ctx->lock);
    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    if (ctx == NULL) {
        PrintError("Invalid context provided to 'SetFinishSignal'.");

        return -1;
    }

    EnterCriticalSection(&ctx->lock);

    ReleaseSemaphore(ctx->finishSignal, THREAD_COUNT, NULL);
    ctx->finishFlag = true;

    LeaveCriticalSection(&ctx->lock);

    return 0;
}

bool GetFinishFlag(Context* ctx) {
    if (ctx == NULL) {
        PrintError("Invalid context provided to 'GetFinishFlag'.");

        return false;
    }

    EnterCriticalSection(&ctx->lock);

    bool flag = ctx->finishFlag;

    LeaveCriticalSection(&ctx->lock);

    return flag;
}