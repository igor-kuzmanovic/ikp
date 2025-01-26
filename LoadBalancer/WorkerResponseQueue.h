#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerResponseQueue(WorkerResponseQueue* queue);

int DestroyWorkerResponseQueue(WorkerResponseQueue* queue);

int PutWorkerResponseQueue(WorkerResponseQueue* queue, const SOCKET workerSocket, const char* data, const int length, const int workerId, const int clientId);

int TakeWorkerResponseQueue(WorkerResponseQueue* queue, WorkerResponse* response);
