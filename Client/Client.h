#pragma once

// User libraries

#include "SharedLibs.h"
#include "InputHandlerThread.h"
#include "ReceiverThread.h"
#include "SenderThread.h"

// API

// Functions

// Cleans up everything
static void CleanupFull(Context* ctx, HANDLE threads[], int threadCount);
