#include "Context.h"

int ContextInitialize(Context* ctx) {
    InitializeCriticalSection(&ctx->lock);

    ctx->finishSignal = CreateSemaphore(0, 0, 2, NULL);
    if (ctx->finishSignal == NULL) {
        return -1;
    }

    ctx->connectSocket = INVALID_SOCKET;

    return 0;
}

int ContextCleanup(Context* ctx) {
    ctx->connectSocket = INVALID_SOCKET;

    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
    }

    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    ReleaseSemaphore(ctx->finishSignal, 2, NULL);
    ctx->finishFlag = true;

    LeaveCriticalSection(&ctx->lock);

    return 0;
}