#include "Context.h"
#include "SharedLibs.h"
#include "ExportThread.h"
#include "ExportQueue.h"

int ContextInitialize(Context* context, int workerId, int peerPort) {
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
    context->finishFlag = 0;  
    context->workerId = workerId;
    context->connectSocket = INVALID_SOCKET;
    context->hashTable = NULL;
    if (InitializeHashTable(&context->hashTable) != 0) {
        PrintCritical("Failed to initialize hash table.");

        return -1;
    }

    context->peerManager = (PeerManager*)malloc(sizeof(PeerManager));
    if (context->peerManager == NULL) {
        PrintCritical("Failed to allocate memory for peer manager.");
        return -1;
    }
    
    if (InitializePeerManager(context->peerManager, peerPort) != 0) {
        PrintCritical("Failed to initialize peer manager.");
        free(context->peerManager);
        context->peerManager = NULL;
        return -1;
    }
    
    context->exportQueue = NULL;
    if (InitializeExportQueue(&context->exportQueue) != 0) {
        PrintCritical("Failed to initialize export queue.");
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
    
    if (context->peerManager != NULL) {
        if (DestroyPeerManager(context->peerManager) != 0) {
            PrintWarning("Failed to destroy peer manager.");
        }
        free(context->peerManager);
        context->peerManager = NULL;
    }
    
    if (context->exportQueue != NULL) {
        if (DestroyExportQueue(context->exportQueue) != 0) {
            PrintWarning("Failed to destroy export queue.");
        }
        context->exportQueue = NULL;
    }
    
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
    context->finishFlag = 0;  
    LeaveCriticalSection(&context->lock);
    DeleteCriticalSection(&context->lock);

    return 0;
}

int SetFinishSignal(Context* context) {
    EnterCriticalSection(&context->lock);

    ReleaseSemaphore(context->finishSignal, THREAD_COUNT, NULL);
    context->finishFlag = 1;
    
    if (context->exportQueue && context->exportQueue->finishSignal) {
        ReleaseSemaphore(context->exportQueue->finishSignal, 1, NULL);
    }

    LeaveCriticalSection(&context->lock);

    return 0;
}

int GetFinishFlag(Context* context) {
    EnterCriticalSection(&context->lock);

    int flag = context->finishFlag;

    LeaveCriticalSection(&context->lock);

    return flag;
}


