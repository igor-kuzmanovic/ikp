#pragma once

#include "SharedLibs.h"

int InitializeWorkerResponseQueue(WorkerResponseQueue* queue);

int DestroyWorkerResponseQueue(WorkerResponseQueue* queue);

int PutWorkerResponseQueue(WorkerResponseQueue* queue, const SOCKET workerSocket, const char* data, const int length, MessageType messageType, const int workerId, const int clientId);

int TakeWorkerResponseQueue(WorkerResponseQueue* queue, WorkerResponse* response);


