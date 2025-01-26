#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerThreadPool(WorkerThreadPool* pool);

int DestroyWorkerThreadPool(WorkerThreadPool* pool);

int AssignWorkerDataReceiverThread(WorkerThreadPool* pool, SOCKET workerSocket, Context* context);

int ReturnWorkerDataReceiverThread(WorkerThreadPool* pool, int threadIndex, WorkerDataReceiverThreadData* data);

