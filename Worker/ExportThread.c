#include "ExportThread.h"

DWORD WINAPI ExportThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;
    ExportQueue* queue = context->exportQueue;

    PrintInfo("Export thread started");

    HANDLE waitHandles[2] = { queue->exportSignal, queue->finishSignal };

    while (true) {
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0 + 1) {
            PrintInfo("Export thread received finish signal");
            break;
        }

        if (waitResult == WAIT_OBJECT_0) {
            EnterCriticalSection(&queue->lock);

            if (queue->requestCount > 0) {
                ExportRequest request = queue->requests[0];

                for (int i = 0; i < queue->requestCount - 1; i++) {
                    queue->requests[i] = queue->requests[i + 1];
                }
                queue->requestCount--;

                LeaveCriticalSection(&queue->lock);

                PrintInfo("Processing export request for worker %u (%s:%u)",
                    request.targetWorkerId, request.targetAddress, request.targetPort);

                int totalEntries = GetHashTableCount(context->hashTable);
                if (totalEntries > 0) {
                    PrintInfo("Exporting %d entries to worker %u", totalEntries, request.targetWorkerId);

                    int exportedCount = 0;
                    int result = ExportHashTableToPeer(context->hashTable, context->peerManager,
                        request.targetWorkerId, &exportedCount);

                    if (result != 0) {
                        PrintError("Failed to export entries to worker %u", request.targetWorkerId);
                    }
                } else {
                    PrintInfo("Hash table is empty, no data to export to worker %u", request.targetWorkerId);
                }
            } else {
                LeaveCriticalSection(&queue->lock);
            }
        }
    }

    PrintInfo("Export thread finished");
    return TRUE;
}
