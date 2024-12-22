#include "Context.h"

int ContextInitialize(Context* ctx) {
    InitializeCriticalSection(&ctx->lock);

    ctx->finishSignal = CreateSemaphore(0, 0, MAX_CLIENTS + MAX_WORKERS + THREAD_COUNT, NULL);
    if (ctx->finishSignal == NULL) {
        return GetLastError();
    }

    ctx->finishFlag = false;

    ctx->clientListenSocket = INVALID_SOCKET;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ctx->clientHandlerThreads[i] = NULL;
    }

    ctx->clientCount = 0;

    ctx->workerListenSocket = INVALID_SOCKET;
    for (int i = 0; i < MAX_WORKERS; i++) {
        ctx->workerHandlerThreads[i] = NULL;
    }

    ctx->workerCount = 0;

    ctx->clientConnectionResultingAddress = NULL;

    ctx->workerConnectionResultingAddress = NULL;

    ctx->clientThreadPool = (ClientThreadPool*)malloc(sizeof(ClientThreadPool));
    if (ctx->clientThreadPool == NULL) {
        return -1;
    }
    InitializeClientThreadPool(ctx->clientThreadPool);

    ctx->clientBlockingRequestQueue = (ClientBlockingRequestQueue*)malloc(sizeof(ClientBlockingRequestQueue));
    if (ctx->clientBlockingRequestQueue == NULL) {
        return -1;
    }
    InitializeClientBlockingRequestQueue(ctx->clientBlockingRequestQueue);

    return 0;
}

int ContextDestroy(Context* ctx) {
    DestroyClientBlockingRequestQueue(ctx->clientBlockingRequestQueue);
    free(ctx->clientBlockingRequestQueue);
    ctx->clientBlockingRequestQueue = NULL;

    DestroyClientThreadPool(ctx->clientThreadPool);
    free(ctx->clientThreadPool);
    ctx->clientThreadPool = NULL;

    ctx->workerConnectionResultingAddress = NULL;

    ctx->clientConnectionResultingAddress = NULL;

    ctx->workerCount = 0;

    for (int i = 0; i < ctx->workerCount; i++) {
        if (ctx->workerHandlerThreads[i] != NULL) {
            CloseHandle(ctx->workerHandlerThreads[i]);
            ctx->workerHandlerThreads[i] = NULL;
        }
    }
    ctx->clientCount = 0;

    for (int i = 0; i < ctx->clientCount; i++) {
        if (ctx->clientHandlerThreads[i] != NULL) {
            CloseHandle(ctx->clientHandlerThreads[i]);
            ctx->clientHandlerThreads[i] = NULL;
        }
    }

    ctx->workerListenSocket = INVALID_SOCKET;

    ctx->clientListenSocket = INVALID_SOCKET;

    ctx->finishFlag = false;

    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
        ctx->finishSignal = NULL;
    }

    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    // TODO Should we track the number of active threads?
    ReleaseSemaphore(ctx->finishSignal, ctx->clientCount + ctx->workerCount + THREAD_COUNT, NULL);
    ctx->finishFlag = true;

    LeaveCriticalSection(&ctx->lock);

    return 0;
}

bool GetFinishFlag(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    bool flag = ctx->finishFlag;

    LeaveCriticalSection(&ctx->lock);

    return flag;
}