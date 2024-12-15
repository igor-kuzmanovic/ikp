#include "Context.h"

int ContextInitialize(Context* ctx) {
    InitializeCriticalSection(&ctx->lock);

    ctx->finishSignal = CreateSemaphore(0, 0, MAX_CLIENTS + 1, NULL);
    if (ctx->finishSignal == NULL) {
        return -1;
    }

    ctx->listenSocket = INVALID_SOCKET;
    ctx->clientCount = 0;

    return 0;
}

int ContextCleanup(Context* ctx) {
    ctx->clientCount = 0;
    ctx->listenSocket = INVALID_SOCKET;

    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
    }

    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    ReleaseSemaphore(ctx->finishSignal, ctx->clientCount + 1, NULL);
    ctx->finishFlag = true;

    LeaveCriticalSection(&ctx->lock);

    return 0;
}