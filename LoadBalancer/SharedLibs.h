#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <conio.h>
#include <windows.h>

// Shared user libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Config.h"

// API

// Structures

typedef struct {
    HANDLE threads[CLIENT_THREAD_POOL_SIZE];
    SOCKET clientSockets[CLIENT_THREAD_POOL_SIZE];
    BOOL isAvailable[CLIENT_THREAD_POOL_SIZE];
    CRITICAL_SECTION lock;
    HANDLE semaphore;
    int count;
} ClientThreadPool;

typedef struct {
    SOCKET clientSocket; // Client socket
    char data[BUFFER_SIZE]; // Data from the client
} ClientRequest;

typedef struct {
    ClientRequest queue[CLIENT_BLOCKING_REQUEST_QUEUE_CAPACITY]; // Circular buffer to store requests
    int head; // Points to the front of the queue
    int tail; // Points to the end of the queue
    int count; // Number of items in the queue
    CRITICAL_SECTION lock; // Protects access to the queue
    HANDLE notEmpty; // Semaphore for consumers
    HANDLE notFull; // Semaphore for producers
} ClientBlockingRequestQueue;

// Structure to hold a shared context
typedef struct {
    CRITICAL_SECTION lock; // Synchronization primitive
    HANDLE finishSignal; // Finish signal
    bool finishFlag; // Stop flag
    SOCKET clientListenSocket; // Client listen socket
    SOCKET workerListenSocket; // Worker listen socket
    HANDLE clientHandlerThreads[MAX_CLIENTS]; // Handles for client threads
    HANDLE workerHandlerThreads[MAX_WORKERS]; // Handles for worker threads
    int clientCount; // Number of active clients
    int workerCount; // Number of active workers
    addrinfo* clientConnectionResultingAddress; // Resulting address information for client connections
    addrinfo* workerConnectionResultingAddress; // Resulting address information for worker connections
    ClientThreadPool* clientThreadPool; // Client thread pool
    ClientBlockingRequestQueue* clientBlockingRequestQueue; // Client blocking request queue
} Context;

typedef struct {
    Context* ctx; // Pointer to the context
    SOCKET clientSocket; // Client socket
    int threadIndex; // Index of the thread in the thread pool
} ClientDataReceiverThreadData;

typedef struct {
    Context* ctx; // Pointer to the context
    SOCKET workerSocket; // Worker socket
} WorkerHandlerThreadData;

// User libraries

#include "Context.h"
#include "ClientThreadPool.h"
#include "ClientBlockingRequestQueue.h"
#include "ClientDataReceiverThread.h"
#include "WorkerHandlerThread.h"
