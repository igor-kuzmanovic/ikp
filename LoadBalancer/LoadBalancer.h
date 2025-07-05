#pragma once

#include "SharedLibs.h"
#include "ClientListenerThread.h"
#include "InputHandlerThread.h"
#include "WorkerListenerThread.h"

static void CleanupFull(Context* context, HANDLE threads[], int threadCount);


