#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

// Initializes a Context
int ContextInitialize(Context* ctx);

// Cleans up a Context
int ContextDestroy(Context* ctx);

// Sets the finish signal
int SetFinishSignal(Context* ctx);

// Gets the finish flag
bool GetFinishFlag(Context* ctx);
