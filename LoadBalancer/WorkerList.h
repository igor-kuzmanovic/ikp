#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeWorkerList(WorkerList* list);

int DestroyWorkerList(WorkerList* list);

int AddWorker(WorkerList* list, SOCKET workerSocket);

int RemoveWorker(WorkerList* list, SOCKET workerSocket);

int GetNextWorker(WorkerList* list, WorkerNode* worker);
