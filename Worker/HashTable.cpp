#include "HashTable.h"

// https://stackoverflow.com/questions/7666509/hash-function-for-string
static unsigned int HashFunction(const char* value) {
    unsigned long hash = 5381;
    const char* valuePointer = value;
    int c;

    while (c = *valuePointer++) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    hash = hash % HASH_TABLE_BUCKET_COUNT;

    return hash;
}

// TODO Remove later or make it actually useful
static void PrintHashTable(const HashTable* table) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'PrintHashTable'.");
        return;
    }

    size_t totalSize = sizeof(HashTable) + table->bucketCount * sizeof(DataNode*) + table->bucketCount * sizeof(CRITICAL_SECTION);
    int totalItems = 0;

    PrintDebug("\nHash Table Status:");
    PrintDebug("------------------------");

    // Iterate through each bucket
    for (int i = 0; i < table->bucketCount; i++) {
        EnterCriticalSection(&table->locks[i]);

        DataNode* node = table->buckets[i];
        size_t bucketSize = 0;
        int bucketCount = 0;

        // Count the key-value pairs in the current bucket
        while (node) {
            bucketSize += strlen(node->key) + 1 + strlen(node->value) + 1; // Size for key, value, and null terminators
            bucketCount++;
            node = node->next;
        }

        // If the bucket has items, print its stats
        if (bucketCount > 0) {
            PrintDebug("Bucket %d: %d items | Bucket Size: %d bytes", i, bucketCount, bucketSize);
        }

        totalItems += bucketCount;
        totalSize += bucketSize;

        LeaveCriticalSection(&table->locks[i]);
    }

    // Print overall memory usage
    size_t totalSizeKB = totalSize / 1024;
    size_t totalSizeMB = totalSizeKB / 1024;

    PrintDebug("------------------------");
    PrintDebug("Total items: %d", totalItems);
    PrintInfo("Total memory usage: %d bytes (%d KB, %d MB)", totalSize, totalSizeKB, totalSizeMB);
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

    newTable->bucketCount = HASH_TABLE_BUCKET_COUNT;

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

    PrintDebug("Hash table initialized with bucket count: %d.", newTable->bucketCount);

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
    table = NULL;

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

            PrintDebug("Key '%s' found in the hash table.", key);

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

            PrintDebug("Value for key '%s' found in the hash table.", key);

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

            PrintDebug("Value for key '%s' updated in the hash table.", key);

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

    PrintDebug("Value for key '%s' set in the hash table.", key);

    // TODO Remove this
    PrintHashTable(table);

    return 1;
}

int DeleteHashTable(const HashTable* table, const char* key) {
    if (table == NULL) {
        PrintError("Invalid hash table provided to 'DeleteHashTable'.");

        return -1;
    }

    if (key == NULL) {
        PrintError("Invalid key provided to 'DeleteHashTable'.");

        return -1;
    }

    if (*key == '\0') {
        PrintError("Empty key provided to 'GetHashTable'.");

        return -1;
    }

    int bucketIndex = HashFunction(key);

    EnterCriticalSection(&table->locks[bucketIndex]);

    DataNode* node = table->buckets[bucketIndex];
    DataNode* prev = NULL;

    while (node) {
        if (strcmp(node->key, key) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                table->buckets[bucketIndex] = node->next;
            }

            if (node->key) {
                free(node->key);
            }

            if (node->value) {
                free(node->value);
            }

            free(node);

            LeaveCriticalSection(&table->locks[bucketIndex]);

            PrintDebug("Value for key '%s' deleted from the hash table.", key);

            return 1;
        }

        prev = node;
        node = node->next;
    }

    LeaveCriticalSection(&table->locks[bucketIndex]);

    return -1;
}