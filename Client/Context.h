#pragma once

#include "SharedLibs.h"

int ContextInitialize(Context* context);
int ContextDestroy(Context* context);
int SetFinishSignal(Context* context);
bool GetFinishFlag(Context* context);
bool GetPauseSender(Context* context);
bool SetPauseSender(Context* context, bool pause);
