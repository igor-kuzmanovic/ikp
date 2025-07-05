#pragma once

#include "SharedLibs.h"

int InitializeClientRequestQueue(ClientRequestQueue* queue);

int DestroyClientRequestQueue(ClientRequestQueue* queue);

int PutClientRequestQueue(ClientRequestQueue* queue, SOCKET clientSocket, const char* data, int length, MessageType messageType, const int clientId);

int TakeClientRequestQueue(ClientRequestQueue* queue, ClientRequest* request);

int ReturnClientRequestQueue(ClientRequestQueue* queue, ClientRequest* request);


