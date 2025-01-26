#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerList(WorkerList* list);

int DestroyWorkerList(WorkerList* list);

int AddWorker(WorkerList* list, const SOCKET workerSocket, const int workerId);

int RemoveWorker(WorkerList* list, const int workerId);

int GetNextWorker(WorkerList* list, WorkerNode* worker);

int IterateWorkersOnce(WorkerList* list, WorkerNode** iterator);

int GetWorkerCount(WorkerList* list);
