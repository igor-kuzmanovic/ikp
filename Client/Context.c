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
    context->pauseSender = false;

    InitializeCriticalSection(&context->testData.lock);
    context->testData.putSuccessCount = 0;
    context->testData.getSuccessCount = 0;
    context->testData.putCount = 0;
    context->testData.getCount = 0;
    context->testData.verificationComplete = 0;

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
    DeleteCriticalSection(&context->testData.lock);
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


