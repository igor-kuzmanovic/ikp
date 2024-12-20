#pragma once

// User libraries

#include "SharedLibs.h"
#include "Config.h"

// API

// Structures

// Functions

void InitializeClientThreadPool(ClientThreadPool* pool);

void DestroyClientThreadPool(ClientThreadPool* pool);

int AssignClientDataReceiverThread(ClientThreadPool* pool, SOCKET clientSocket, Context *ctx);

void ReturnClientDataReceiverThread(ClientThreadPool* pool, int threadIndex, ClientDataReceiverThreadData* data);
