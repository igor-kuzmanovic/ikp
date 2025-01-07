#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

// Initializes a Context
int ContextInitialize(Context* context);

// Cleans up a Context
int ContextDestroy(Context* context);

// Sets the finish signal
int SetFinishSignal(Context* context);

// Gets the finish flag
bool GetFinishFlag(Context* context);
