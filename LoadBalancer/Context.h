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
    SOCKET listenSocket; // Listen socket
    HANDLE clientThreads[MAX_CLIENTS]; // Handles for client threads
    int clientCount; // Number of active clients
    addrinfo* resultingAddress; // Resulting address information
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
