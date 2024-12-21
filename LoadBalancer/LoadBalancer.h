#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"
#include "ClientListenerThread.h"
#include "InputHandlerThread.h"
#include "WorkerHandlerThread.h"
#include "WorkerListenerThread.h"

// API

// Functions

// Cleans up everything
static void CleanupFull(Context* ctx, HANDLE threads[], int threadCount);
