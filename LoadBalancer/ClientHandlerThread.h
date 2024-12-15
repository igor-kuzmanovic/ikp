#pragma once

// User libraries

#include "LoadBalancer.h"
#include "Context.h"

// API

// Structures

typedef struct {
    SOCKET clientSocket; // Client socket
    Context* ctx; // Pointer to the context
} ClientHandlerThreadData;

// Functions

DWORD WINAPI ClientHandlerThread(LPVOID lpParam);
