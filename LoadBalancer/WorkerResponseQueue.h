#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerResponseQueue(WorkerResponseQueue* queue);

int DestroyWorkerResponseQueue(WorkerResponseQueue* queue);

int PutWorkerResponseQueue(WorkerResponseQueue* queue, SOCKET workerSocket, const char* data, int length);

int TakeWorkerResponseQueue(WorkerResponseQueue* queue, WorkerResponse* response);
