#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"

// API

// Structures

// Functions

// Initializes a Context
int ContextInitialize(Context* ctx);

// Cleans up a Context
int ContextDestroy(Context* ctx);

// Sets the finish signal
int SetFinishSignal(Context* ctx);

// Gets the finish flag
bool GetFinishFlag(Context* ctx);
