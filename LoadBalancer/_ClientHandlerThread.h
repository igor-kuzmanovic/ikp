#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Structures

typedef struct {
    SOCKET clientSocket; // Client socket
    Context* context; // Pointer to the context
} ClientHandlerThreadData;

// Functions

DWORD WINAPI ClientHandlerThread(LPVOID lpParam);
