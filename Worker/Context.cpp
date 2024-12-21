#include "Context.h"

int ContextInitialize(Context* ctx) {
    InitializeCriticalSection(&ctx->lock);
    ctx->finishSignal = CreateSemaphore(0, 0, THREAD_COUNT, NULL);
    if (ctx->finishSignal == NULL) {
        return GetLastError();
    }
    ctx->finishFlag = false;
    ctx->connectSocket = INVALID_SOCKET;

    return 0;
}

int ContextDestroy(Context* ctx) {
    ctx->connectSocket = INVALID_SOCKET;
    if (ctx->finishSignal != NULL) {
        CloseHandle(ctx->finishSignal);
        ctx->finishSignal = NULL;
    }
    ctx->finishFlag = false;
    DeleteCriticalSection(&ctx->lock);

    return 0;
}

int SetFinishSignal(Context* ctx) {
    EnterCriticalSection(&ctx->lock);

    // TODO Should we track the number of active threads?
    ReleaseSemaphore(ctx->finishSignal, THREAD_COUNT, NULL);
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