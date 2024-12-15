#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"

// API

// Structures

typedef struct {
    SOCKET workerSocket; // Worker socket
    Context* ctx; // Pointer to the context
} WorkerHandlerThreadData;

// Functions

DWORD WINAPI WorkerHandlerThread(LPVOID lpParam);
