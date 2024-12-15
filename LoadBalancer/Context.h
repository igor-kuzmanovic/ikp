#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"

// API

// Structures

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
} Context;

// Functions

// Initializes a Context
int ContextInitialize(Context* ctx);

// Cleans up a Context
int ContextCleanup(Context* ctx);

// Sets the finish signal
int SetFinishSignal(Context* ctx);

// Gets the finish flag
bool GetFinishFlag(Context* ctx);
