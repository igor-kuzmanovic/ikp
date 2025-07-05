#pragma once

#include "SharedLibs.h"

int ContextInitialize(Context* context, int workerId, int peerPort);
int ContextDestroy(Context* context);

int SetFinishSignal(Context* context);

int GetFinishFlag(Context* context);


