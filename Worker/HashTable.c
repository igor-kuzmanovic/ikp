#include "HashTable.h"
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "../Lib/Protocol.h"
#include "SharedLibs.h"

static unsigned int HashFunction(const char* value) {
    unsigned long hash = 5381;
    const char* valuePointer = value;
    int c;

    while (c = *valuePointer++) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    hash = hash % WORKER_HASH_TABLE_BUCKET_COUNT;

    return hash;
}

void GetHashTableStats(const HashTable* table, uint32_t* memoryUsage, int* itemCount) {
    if (table == NULL) {
        if (memoryUsage) *memoryUsage = 0;
        if (itemCount) *itemCount = 0;
        PrintError("Invalid hash table provided to 'GetHashTableStats'.");
        return;
    }

    uint32_t totalSize = sizeof(HashTable) + table->bucketCount * sizeof(DataNode*) + table->bucketCount * sizeof(CRITICAL_SECTION);
    int totalItems = 0;
    
    for (int i = 0; i < table->bucketCount; i++) {
        EnterCriticalSection(&table->locks[i]);
        
        DataNode* node = table->buckets[i];
        while (node) {
            totalSize += sizeof(DataNode);
            totalSize += (uint32_t)(strlen(node->key) + 1);
            totalSize += (uint32_t)(strlen(node->value) + 1);
            totalItems++;
            node = node->next;
        }
        
        LeaveCriticalSection(&table->locks[i]);
    }
    
    if (memoryUsage) *memoryUsage = totalSize;
    if (itemCount) *itemCount = totalItems;
}

int InitializeHashTable(HashTable** table) {
    if (table == NULL) {
        PrintError("Invalid hash table pointer provided to 'InitializeHashTable'.");

        return -1;
    }

    HashTable* newTable = (HashTable*)malloc(sizeof(HashTable));
    if (newTable == NULL) {
        PrintError("Failed to allocate memory for a new hash table.");

        return -1;
    }

    newTable->bucketCount = WORKER_HASH_TABLE_BUCKET_COUNT;

    newTable->buckets = (DataNode**)calloc(newTable->bucketCount, sizeof(DataNode*));
    if (newTable->buckets == NULL) {
        PrintError("Failed to allocate memory for hash table buckets.");

        free(newTable);

        return -1;
    }

    newTable->locks = (CRITICAL_SECTION*)malloc(newTable->bucketCount * sizeof(CRITICAL_SECTION));
    if (newTable->locks == NULL) {
        PrintError("Failed to allocate memory for hash table locks.");

        free(newTable->buckets);
        free(newTable);

        return -1;
    }

    for (int i = 0; i < newTable->bucketCount; i++) {
        InitializeCriticalSection(&newTable->locks[i]);
    }

    *table = newTable;

    return 0;
}

int DestroyHashTable(HashTable* table) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'DestroyHashTable'.");

        return -1;
    }

    for (int i = 0; i < table->bucketCount; i++) {
        EnterCriticalSection(&table->locks[i]);

        DataNode* node = table->buckets[i];
        while (node) {
            DataNode* toDelete = node;
            node = node->next;

            if (toDelete->key) {
                free(toDelete->key);
                toDelete->key = NULL;
            }

            if (toDelete->value) {
                free(toDelete->value);
                toDelete->value = NULL;
            }

            free(toDelete);
        }

        LeaveCriticalSection(&table->locks[i]);
        DeleteCriticalSection(&table->locks[i]);
    }

    free(table->buckets);
    free(table->locks);
    free(table);

    return 0;
}

int HasHashTable(const HashTable* table, const char* key) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'GetHashTable'.");

        return -1;
    }

    if (key == NULL) {
        PrintError("Invalid key provided to 'GetHashTable'.");

        return -1;
    }

    if (*key == '\0') {
        PrintError("Empty key provided to 'GetHashTable'.");

        return -1;
    }

    int bucketIndex = HashFunction(key);

    EnterCriticalSection(&table->locks[bucketIndex]);

    DataNode* node = table->buckets[bucketIndex];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            LeaveCriticalSection(&table->locks[bucketIndex]);

            return 1;
        }

        node = node->next;
    }

    LeaveCriticalSection(&table->locks[bucketIndex]);

    return 0;
}

int GetHashTable(const HashTable* table, const char* key, char** value) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'GetHashTable'.");

        return -1;
    }

    if (key == NULL) {
        PrintError("Invalid key provided to 'GetHashTable'.");

        return -1;
    }

    if (*key == '\0') {
        PrintError("Empty key provided to 'GetHashTable'.");

        return -1;
    }

    if (value == NULL) {
        PrintError("Invalid value pointer provided to 'GetHashTable'.");

        return -1;
    }

    int iResult;

    int bucketIndex = HashFunction(key);

    EnterCriticalSection(&table->locks[bucketIndex]);

    DataNode* node = table->buckets[bucketIndex];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            *value = (char*)malloc(strlen(node->value) + 1);
            if (*value == NULL) {
                PrintError("Failed to allocate memory for hash table value.");

                LeaveCriticalSection(&table->locks[bucketIndex]);

                return -1;
            }

            iResult = strcpy_s(*value, strlen(node->value) + 1, node->value);
            if (iResult != 0) {
                PrintError("Failed to copy hash table value.");

                free(*value);

                LeaveCriticalSection(&table->locks[bucketIndex]);

                return -1;
            }

            LeaveCriticalSection(&table->locks[bucketIndex]);

            return 1;
        }

        node = node->next;
    }

    LeaveCriticalSection(&table->locks[bucketIndex]);

    return 0;
}

int SetHashTable(HashTable* table, const char* key, const char* value) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'GetHashTable'.");

        return -1;
    }

    if (key == NULL) {
        PrintError("Invalid key provided to 'GetHashTable'.");

        return -1;
    }

    if (*key == '\0') {
        PrintError("Empty key provided to 'GetHashTable'.");

        return -1;
    }

    if (value == NULL) {
        PrintError("Invalid value pointer provided to 'GetHashTable'.");

        return -1;
    }

    int iResult;

    int bucketIndex = HashFunction(key);

    EnterCriticalSection(&table->locks[bucketIndex]);

    DataNode* node = table->buckets[bucketIndex];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            free(node->value);

            node->value = (char*)malloc(strlen(value) + 1);
            if (node->value == NULL) {
                PrintError("Failed to allocate memory for hash table value.");

                LeaveCriticalSection(&table->locks[bucketIndex]);

                return -1;
            }

            iResult = strcpy_s(node->value, strlen(value) + 1, value);
            if (iResult != 0) {
                PrintError("Failed to copy hash table value.");

                LeaveCriticalSection(&table->locks[bucketIndex]);

                return -1;
            }

            LeaveCriticalSection(&table->locks[bucketIndex]);

            return 1;
        }

        node = node->next;
    }

    DataNode* newNode = (DataNode*)malloc(sizeof(DataNode));
    if (newNode == NULL) {
        PrintError("Failed to allocate memory for a new hash table node.");

        LeaveCriticalSection(&table->locks[bucketIndex]);

        return -1;
    }

    newNode->key = (char*)malloc(strlen(key) + 1);
    if (newNode->key == NULL) {
        PrintError("Failed to allocate memory for hash table key.");

        free(newNode);

        LeaveCriticalSection(&table->locks[bucketIndex]);

        return -1;
    }

    iResult = strcpy_s(newNode->key, strlen(key) + 1, key);
    if (iResult != 0) {
        PrintError("Failed to copy hash table key.");

        free(newNode->key);
        free(newNode);

        LeaveCriticalSection(&table->locks[bucketIndex]);

        return -1;
    }

    newNode->value = (char*)malloc(strlen(value) + 1);
    if (newNode->value == NULL) {
        PrintError("Failed to allocate memory for hash table value.");

        free(newNode->key);
        free(newNode);

        LeaveCriticalSection(&table->locks[bucketIndex]);

        return -1;
    }

    iResult = strcpy_s(newNode->value, strlen(value) + 1, value);
    if (iResult != 0) {
        PrintError("Failed to copy hash table value.");

        free(newNode->key);
        free(newNode->value);
        free(newNode);

        LeaveCriticalSection(&table->locks[bucketIndex]);

        return -1;
    }

    newNode->next = table->buckets[bucketIndex];
    table->buckets[bucketIndex] = newNode;

    LeaveCriticalSection(&table->locks[bucketIndex]);

    return 1;
}

int GetHashTableCount(const HashTable* table) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'GetHashTableCount'.");
        return -1;
    }

    int itemCount;
    GetHashTableStats(table, NULL, &itemCount);
    return itemCount;
}

int ExportHashTableToPeer(const HashTable* table, PeerManager* peerManager, uint32_t targetWorkerId, int* exportedCount) {
    if (table == NULL || peerManager == NULL || exportedCount == NULL) {
        PrintError("Invalid parameters provided to 'ExportHashTableToPeer'.");
        return -1;
    }

    *exportedCount = 0;

    SOCKET peerSocket = GetPeerConnection(peerManager, targetWorkerId);
    if (peerSocket == INVALID_SOCKET) {
        PrintError("No connection to worker %d for data export", targetWorkerId);
        return -1;
    }

    if (SendDataExportStart(peerSocket, GetHashTableCount(table)) <= 0) {
        PrintError("Failed to send data export start to worker %d", targetWorkerId);
        return -1;
    }

    for (int i = 0; i < table->bucketCount; i++) {
        EnterCriticalSection(&table->locks[i]);
        
        DataNode* node = table->buckets[i];
        while (node) {
            if (SendDataEntry(peerSocket, node->key, node->value) <= 0) {
                PrintError("Failed to send data entry to worker %d: %s", targetWorkerId, node->key);
                LeaveCriticalSection(&table->locks[i]);
                return -1;
            }
            (*exportedCount)++;
            node = node->next;
        }
        
        LeaveCriticalSection(&table->locks[i]);
        
        if (i % 10 == 0 && i > 0) {
            Sleep(WORKER_DATA_EXPORT_BUCKET_YIELD_SLEEP_TIME);
        }
    }

    if (SendDataExportEnd(peerSocket) <= 0) {
        PrintError("Failed to send data export end to worker %d", targetWorkerId);
        return -1;
    }
    
    PrintInfo("Successfully exported %d entries to worker %d", *exportedCount, targetWorkerId);
    return 0;
}


