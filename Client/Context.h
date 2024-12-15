#pragma once

// User libraries

#include "NetworkLib.h"
#include "LoggingLib.h"

// API

// Structures

// Structure to hold a shared context
typedef struct {
    CRITICAL_SECTION lock; // Synchronization primitive
    HANDLE finishSignal; // Finish signal
    bool finishFlag; // Stop flag
    SOCKET connectSocket; // Connect socket
} Context;

// Functions

// Initializes a Context
int ContextInitialize(Context* ctx);

// Cleans up a Context
int ContextCleanup(Context* ctx);

// Sets the finish signal
int SetFinishSignal(Context* ctx);
