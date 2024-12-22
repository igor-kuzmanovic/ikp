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

// Gets the pause sender flag
bool GetPauseSender(Context* ctx);

// Sets the pause sender flag
bool SetPauseSender(Context* ctx, bool pause);