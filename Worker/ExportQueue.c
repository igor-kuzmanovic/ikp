#include "ExportQueue.h"

int InitializeExportQueue(ExportQueue** queue) {
    *queue = (ExportQueue*)malloc(sizeof(ExportQueue));
    if (*queue == NULL) {
        return -1;
    }
    
    memset(*queue, 0, sizeof(ExportQueue));
    InitializeCriticalSection(&(*queue)->lock);
    
    (*queue)->exportSignal = CreateSemaphore(NULL, 0, MAX_WORKERS, NULL);
    if ((*queue)->exportSignal == NULL) {
        DeleteCriticalSection(&(*queue)->lock);
        free(*queue);
        *queue = NULL;
        return -1;
    }
    
    (*queue)->finishSignal = CreateSemaphore(NULL, 0, 1, NULL);
    if ((*queue)->finishSignal == NULL) {
        CloseHandle((*queue)->exportSignal);
        DeleteCriticalSection(&(*queue)->lock);
        free(*queue);
        *queue = NULL;
        return -1;
    }
    
    return 0;
}

int DestroyExportQueue(ExportQueue* queue) {
    if (queue == NULL) return 0;
    
    EnterCriticalSection(&queue->lock);
    
    if (queue->exportSignal != NULL) {
        CloseHandle(queue->exportSignal);
    }
    if (queue->finishSignal != NULL) {
        CloseHandle(queue->finishSignal);
    }
    
    LeaveCriticalSection(&queue->lock);
    DeleteCriticalSection(&queue->lock);
    free(queue);
    
    return 0;
}

int AddExportRequest(ExportQueue* queue, uint32_t targetWorkerId, const char* address, uint16_t port) {
    if (queue == NULL || address == NULL) return -1;
    
    EnterCriticalSection(&queue->lock);
    
    if (queue->requestCount >= MAX_WORKERS) {
        LeaveCriticalSection(&queue->lock);
        PrintWarning("Export queue is full, cannot add request for worker %u", targetWorkerId);
        return -1;
    }
    
    ExportRequest* request = &queue->requests[queue->requestCount];
    request->targetWorkerId = targetWorkerId;
    strncpy_s(request->targetAddress, sizeof(request->targetAddress), address, strlen(address));
    request->targetPort = port;
    
    queue->requestCount++;
    
    LeaveCriticalSection(&queue->lock);
    
    ReleaseSemaphore(queue->exportSignal, 1, NULL);
    
    PrintDebug("Added export request for worker %u (%s:%u)", targetWorkerId, address, port);
    return 0;
}
