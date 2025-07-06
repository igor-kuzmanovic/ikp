#pragma once

#include "SharedLibs.h"
#include "InputHandlerThread.h"
#include "ReceiverThread.h"
#include "SenderThread.h"

static void CleanupFull(Context* context, HANDLE threads[], int threadCount);
