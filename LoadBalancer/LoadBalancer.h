#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"
#include "Context.h"
#include "ClientHandlerThread.h"
#include "InputHandlerThread.h"

// API

// Functions

// Cleans up everything
static void CleanupFull(Context* ctx, HANDLE threads[], int threadCount);
