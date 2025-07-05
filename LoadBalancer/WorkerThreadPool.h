#pragma once

#include "SharedLibs.h"

int InitializeWorkerThreadPool(WorkerThreadPool* pool);

int DestroyWorkerThreadPool(WorkerThreadPool* pool);

int AssignWorkerDataReceiverThread(WorkerThreadPool* pool, const SOCKET workerSocket,  Context* context, const int workerId);

int ReturnWorkerDataReceiverThread(WorkerThreadPool* pool, const int threadIndex, WorkerDataReceiverThreadData* data);


