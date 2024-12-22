#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

void InitializeClientBlockingRequestQueue(ClientBlockingRequestQueue* queue);

void DestroyClientBlockingRequestQueue(ClientBlockingRequestQueue* queue);

int PutClientBlockingRequestQueue(ClientBlockingRequestQueue* queue, SOCKET clientSocket, const char* data);

ClientRequest TakeClientBlockingRequestQueue(ClientBlockingRequestQueue* queue);
