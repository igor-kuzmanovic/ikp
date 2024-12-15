#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include "conio.h"

// User libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "ClientHandlerThread.h"
#include "InputHandlerThread.h"

// User-defined constants

#define BUFFER_SIZE 512

#define SERVER_PORT "5059"

#define MAX_CLIENTS 4

// Load balancer context

// Structure to hold a shared context
typedef struct {
    bool stopServer; // Stop flag
    SOCKET listenSocket; // Listening socket
    HANDLE clientThreads[MAX_CLIENTS]; // Handles for client threads
    int clientCount; // Number of active clients
    CRITICAL_SECTION lock; // Synchronization primitive
} LoadBalancerContext;

// Initializes a LoadBalancerContext
int LoadBalancerContextInitialize(LoadBalancerContext* ctx);

// Cleans up a LoadBalancerContext
int LoadBalancerContextCleanup(LoadBalancerContext* ctx);

// Client handler thread data
typedef struct {
    SOCKET clientSocket; // Client socket
    LoadBalancerContext* ctx; // Pointer to the shared context
} ClientHandlerThreadData;
