#pragma once

// User libraries

#include "SharedLibs.h"
#include "ClientListenerThread.h"
#include "InputHandlerThread.h"
#include "WorkerListenerThread.h"
#include "WorkerHealthCheckThread.h"

// API

// Functions

// Cleans up everything
static void CleanupFull(Context* context, HANDLE threads[], int threadCount);
