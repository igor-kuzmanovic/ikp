#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <conio.h>
#include <windows.h>

// Shared user libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "SharedConfig.h"
#include "Config.h"

// Logging

// https://stackoverflow.com/questions/26053959/what-does-va-args-in-a-macro-mean
#define LOGGING_NAMESPACE "LB"
#define PrintDebug(format, ...) PrintDebug(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintInfo(format, ...) PrintInfo(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintWarning(format, ...) PrintWarning(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintError(format, ...) PrintError(LOGGING_NAMESPACE, format, __VA_ARGS__)
#define PrintCritical(format, ...) PrintCritical(LOGGING_NAMESPACE, format, __VA_ARGS__)

// API

// Structures

typedef struct WorkerNode {
    int id;                     // Worker id
    SOCKET socket;              // Worker socket
    struct WorkerNode* next;    // Next node in the list
    struct WorkerNode* prev;    // Previous node in the list
} WorkerNode;

typedef struct {
    WorkerNode* head;       // Head of the circular list
    WorkerNode* current;    // Pointer for Round Robin traversal
    CRITICAL_SECTION lock;  // Protects access to the list
    HANDLE notEmpty;        // Not empty signal
    HANDLE notFull;         // Not full signal
    int count;              // Number of workers
    int nextGeneratedId;    // Next generated worker id
} WorkerList;

typedef struct {
    HANDLE threads[MAX_CLIENTS];        // Client threads
    SOCKET clientSockets[MAX_CLIENTS];  // Client sockets
    BOOL isAvailable[MAX_CLIENTS];      // Availability of the threads
    CRITICAL_SECTION lock;                          // Protects access to the pool
    HANDLE semaphore;                               // Semaphore for synchronization
    int count;                                      // Number of threads
} ClientThreadPool;

typedef struct {
    SOCKET clientSocket;    // Client socket
    char data[BUFFER_SIZE]; // Data from the client
} ClientRequest;

typedef struct {
    ClientRequest queue[CLIENT_REQUEST_QUEUE_CAPACITY]; // Circular buffer to store requests
    int head;                                           // Points to the front of the queue
    int tail;                                           // Points to the end of the queue
    int count;                                          // Number of items in the queue
    CRITICAL_SECTION lock;                              // Protects access to the queue
    HANDLE notEmpty;                                    // Not empty signal for consumers
    HANDLE notFull;                                     // Not full signal for producers
} ClientRequestQueue;

// Structure to hold a shared context
typedef struct {
    CRITICAL_SECTION lock;                      // Protects access to the context
    HANDLE finishSignal;                        // Finish signal
    bool finishFlag;                            // Stop flag
    SOCKET clientListenSocket;                  // Client listen socket
    SOCKET workerListenSocket;                  // Worker listen socket
    addrinfo* clientConnectionResultingAddress; // Resulting address information for client connections
    addrinfo* workerConnectionResultingAddress; // Resulting address information for worker connections
    WorkerList* workerList;                     // Worker list
    ClientThreadPool* clientThreadPool;         // Client thread pool
    ClientRequestQueue* clientRequestQueue;     // Client request queue
} Context;

typedef struct {
    Context* context;           // Pointer to the context
    SOCKET clientSocket;    // Client socket
    int threadIndex;        // Index of the thread in the thread pool
} ClientDataReceiverThreadData;

typedef struct {
    Context* context;           // Pointer to the context
    SOCKET workerSocket;    // Worker socket
} WorkerHandlerThreadData;

// User libraries

#include "Context.h"
#include "ClientThreadPool.h"
#include "ClientRequestQueue.h"
#include "ClientDataReceiverThread.h"
#include "WorkerList.h"
#include "WorkerClientRequestDispatcherThread.h"
