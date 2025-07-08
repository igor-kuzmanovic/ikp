#include "InputHandlerThread.h"

DWORD WINAPI InputHandlerThread(LPVOID lpParam) {

    Context* context = (Context*)lpParam;

    PrintInfo("Press 'q' to stop the worker, 's' to show hashtable status.");

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        if (_kbhit()) { 
            char ch = _getch();
            
            while (_kbhit()) {
                _getch();
            }
            
            if (ch == 'q' || ch == 'Q') {
                PrintInfo("Shutdown signal received.");

                SetFinishSignal(context);

                break;
            } else if (ch == 's' || ch == 'S') {
                PrintInfo("=== HashTable Status ===");
                uint32_t totalSize;
                int totalItems;
                GetHashTableStats(context->hashTable, &totalSize, &totalItems);
                uint32_t totalSizeKB = totalSize / 1024;
                uint32_t totalSizeMB = totalSizeKB / 1024;
                double avgItemsPerBucket = (double)totalItems / WORKER_HASH_TABLE_BUCKET_COUNT;
                PrintInfo("Total memory usage: %u bytes (%u KB, %u MB)", totalSize, totalSizeKB, totalSizeMB);
                PrintInfo("Total items: %d, Buckets: %d, Avg items/bucket: %.2f", 
                         totalItems, WORKER_HASH_TABLE_BUCKET_COUNT, avgItemsPerBucket);
                PrintInfo("========================");
            }
        }

        Sleep(INPUT_WAIT_TIME);
    }

    return TRUE;
}


