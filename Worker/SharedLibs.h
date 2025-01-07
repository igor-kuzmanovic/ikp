#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <conio.h>
#include <windows.h>

// Shared user libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Protocol.h"
#include "SharedConfig.h"
#include "Config.h"

// Logging

// https://stackoverflow.com/questions/26053959/what-does-va-args-in-a-macro-mean
#define LOGGING_NAMESPACE "WR"
#define PrintDebug(format, ...) PrintDebug(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintInfo(format, ...) PrintInfo(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintWarning(format, ...) PrintWarning(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintError(format, ...) PrintError(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintCritical(format, ...) PrintCritical(LOGGING_NAMESPACE, format, __VA_ARGS__)

// API

// Structures

typedef struct DataNode {
    char* key;                  // Dynamically allocated string for the key
    char* value;                // Dynamically allocated string for the value
    struct DataNode* next;      // Pointer to the next node for collision handling (chaining)
} DataNode;

typedef struct {
    DataNode** buckets;         // Array of pointers to DataNode (hash buckets)
    int bucketCount;            // Number of buckets in the table
    CRITICAL_SECTION* locks;    // Array of locks (one per bucket for fine-grained locking)
} HashTable;

typedef struct {
    CRITICAL_SECTION lock;      // Synchronization primitive
    HANDLE finishSignal;        // Finish signal
    bool finishFlag;            // Finish flag
    SOCKET connectSocket;       // Connect socket
    HashTable* hashTable;       // Hash table
} Context;

// User libraries

#include "Context.h"
#include "HashTable.h"
