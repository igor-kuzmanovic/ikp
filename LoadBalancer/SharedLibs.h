#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <conio.h>
#include <windows.h>

// User libraries

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
} Context;

typedef struct {
    SOCKET clientSocket; // Client socket
    Context* ctx; // Pointer to the context
    int threadIndex; // Index of the thread in the thread pool
} ClientDataReceiverThreadData;

// User libraries

#include "Context.h"
#include "ClientThreadPool.h"
#include "ClientDataReceiverThread.h"