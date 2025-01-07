#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeClientRequestQueue(ClientRequestQueue* queue);

int DestroyClientRequestQueue(ClientRequestQueue* queue);

int PutClientRequestQueue(ClientRequestQueue* queue, SOCKET clientSocket, const char* data, int length);

int TakeClientRequestQueue(ClientRequestQueue* queue, ClientRequest* request);
