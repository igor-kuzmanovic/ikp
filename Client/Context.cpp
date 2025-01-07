#include "Context.h"

int ContextInitialize(Context* context) {
    InitializeCriticalSection(&context->lock);
    context->finishSignal = CreateSemaphore(0, 0, THREAD_COUNT, NULL);
    if (context->finishSignal == NULL) {
        PrintCritical("Failed to create a semaphore for the finish signal.");

        return GetLastError();
    }
    context->finishFlag = false;
    context->connectSocket = INVALID_SOCKET;

    return 0;
}

int ContextDestroy(Context* context) {
    EnterCriticalSection(&context->lock);
    context->connectSocket = INVALID_SOCKET;
    if (context->finishSignal != NULL) {
        CloseHandle(context->finishSignal);
        context->finishSignal = NULL;
    }
    context->finishFlag = false;
    LeaveCriticalSection(&context->lock);
    DeleteCriticalSection(&context->lock);

    return 0;
}

int SetFinishSignal(Context* context) {
    EnterCriticalSection(&context->lock);

    // TODO Should we track the number of active threads?
    ReleaseSemaphore(context->finishSignal, THREAD_COUNT, NULL);
    context->finishFlag = true;

    LeaveCriticalSection(&context->lock);

    return 0;
}

bool GetFinishFlag(Context* context) {
    EnterCriticalSection(&context->lock);

    bool flag = context->finishFlag;

    LeaveCriticalSection(&context->lock);

    return flag;
}

bool GetPauseSender(Context* context) {
    EnterCriticalSection(&context->lock);

    bool pause = context->pauseSender;

    LeaveCriticalSection(&context->lock);

    return pause;
}

bool SetPauseSender(Context* context, bool pause) {
    EnterCriticalSection(&context->lock);

    context->pauseSender = pause;

    LeaveCriticalSection(&context->lock);

    return true;
}