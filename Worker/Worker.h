#pragma once

#include "SharedLibs.h"
#include "InputHandlerThread.h"
#include "ReceiverThread.h"
#include "PeerListenerThread.h"

static void CleanupFull(Context* context, HANDLE threads[], int threadCount);


