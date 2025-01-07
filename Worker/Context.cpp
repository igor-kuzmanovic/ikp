#include "Context.h"

int ContextInitialize(Context* context) {
    if (context == NULL) {
        PrintError("Invalid context provided to 'ContextInitialize'.");

        return -1;
    }

    InitializeCriticalSection(&context->lock);
    context->finishSignal = CreateSemaphore(0, 0, THREAD_COUNT, NULL);
    if (context->finishSignal == NULL) {
        PrintCritical("Failed to create a semaphore for the finish signal.");

        return GetLastError();
    }
    context->finishFlag = false;
    context->connectSocket = INVALID_SOCKET;
    context->hashTable = NULL;
    if (InitializeHashTable(&context->hashTable) != 0) {
        PrintCritical("Failed to initialize hash table.");

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
    if (context->hashTable != NULL) {
        if (DestroyHashTable(context->hashTable) != 0) {
            PrintWarning("Failed to destroy hash table.");
        }
        context->hashTable = NULL;
    }
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