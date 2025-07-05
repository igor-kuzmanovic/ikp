#pragma once

#include "SharedLibs.h"

int ContextInitialize(Context* context);


int ContextDestroy(Context* context);

int SetFinishSignal(Context* context);

int GetFinishFlag(Context* context);

int GetWorkerAddress(SOCKET socket, char* addressBuffer, size_t bufferSize);


