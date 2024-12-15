#include "Context.h"

int ContextInitialize(Context* ctx) {
    InitializeCriticalSection(&ctx->lock);
    ctx->finishSignal = CreateSemaphore(0, 0, MAX_CLIENTS + THREAD_COUNT, NULL);
    if (ctx->finishSignal == NULL) {
        return -1;
    }
    ctx->finishFlag = false;
    ctx->listenSocket = INVALID_SOCKET;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ctx->clientThreads[i] = NULL;
    }
    ctx->clientCount = 0;
    ctx->resultingAddress = NULL;

    return 0;
}

int ContextCleanup(Context* ctx) {
    ctx->resultingAddress = NULL;
    ctx->clientCount = 0;
    for (int i = 0; i < ctx->clientCount; i++) {
        if (ctx->clientThreads[i] != NULL) {
            CloseHandle(ctx->clientThreads[i]);
            ctx->clientThreads[i] = NULL;
        }
    }
    ctx->listenSocket = INVALID_SOCKET;
    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
        ctx->finishSignal = NULL;
    }
    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    ReleaseSemaphore(ctx->finishSignal, ctx->clientCount + THREAD_COUNT, NULL);
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