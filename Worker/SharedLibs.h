#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <conio.h>
#include <windows.h>

// Shared user libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Config.h"

// API

// Structures

typedef struct {
    CRITICAL_SECTION lock; // Synchronization primitive
    HANDLE finishSignal; // Finish signal
    bool finishFlag; // Finish flag
    SOCKET connectSocket; // Connect socket
} Context;

// User libraries

#include "Context.h"
