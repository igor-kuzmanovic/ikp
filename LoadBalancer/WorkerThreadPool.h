#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerThreadPool(WorkerThreadPool* pool);

int DestroyWorkerThreadPool(WorkerThreadPool* pool);

int AssignWorkerDataReceiverThread(WorkerThreadPool* pool, const SOCKET workerSocket,  Context* context, const int workerId);

int ReturnWorkerDataReceiverThread(WorkerThreadPool* pool, const int threadIndex, WorkerDataReceiverThreadData* data);

