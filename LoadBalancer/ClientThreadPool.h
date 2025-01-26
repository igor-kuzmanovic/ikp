#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeClientThreadPool(ClientThreadPool* pool);

int DestroyClientThreadPool(ClientThreadPool* pool);

int AssignClientDataReceiverThread(ClientThreadPool* pool, SOCKET clientSocket, Context *context, int clientId);

int ReturnClientDataReceiverThread(ClientThreadPool* pool, int threadIndex, ClientDataReceiverThreadData* data);

int GetClientSocketByClientId(const ClientThreadPool* pool, const int clientId, SOCKET* clientSocket);
