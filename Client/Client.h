#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include "conio.h"

// User libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "InputHandlerThread.h"
#include "ReceiverThread.h"
#include "SenderThread.h"

// User-defined constants

#define BUFFER_SIZE 512

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 5059

// Client context

// Structure to hold a shared context
typedef struct {
    bool stopClient; // Stop flag
    SOCKET connectSocket; // Connect socket
    CRITICAL_SECTION lock; // Synchronization primitive
} ClientContext;

// Initializes a ClientContext
int ClientContextInitialize(ClientContext* ctx);

// Cleans up a ClientContext
int ClientContextCleanup(ClientContext* ctx);
