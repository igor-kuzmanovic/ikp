#pragma once

#include "SharedLibs.h"

int InitializeWorkerList(WorkerList* list);

int DestroyWorkerList(WorkerList* list);

int AddWorker(WorkerList* list, const SOCKET workerSocket, const int workerId, const char* workerAddress, const int workerPeerPort);

int RemoveWorker(WorkerList* list, const int workerId);

int GetNextWorker(WorkerList* list, WorkerNode* worker);

int IterateWorkersOnce(WorkerList* list, WorkerNode** iterator);

int GetWorkerCount(WorkerList* list);

int SetWorkerReady(WorkerList* list, int workerId);

int UpdateWorkerPeerPort(WorkerList* list, int workerId, int peerPort);

int SendWorkerRegistryToSingleWorker(WorkerList* list, int targetWorkerId);

int GetWorkerById(WorkerList* list, int workerId, WorkerNode* worker);

int SendWorkerRegistryEntries(SOCKET socket, WorkerList* list);

int BroadcastNewWorkerJoined(WorkerList* list, int newWorkerId);
