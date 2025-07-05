#pragma once

#include "../Lib/SharedConfig.h"

#include <conio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Protocol.h"
#include "SharedConfig.h"
#include "Config.h"

#define LOGGING_NAMESPACE "LB"

#undef PrintDebug
#undef PrintInfo
#undef PrintWarning
#undef PrintError
#undef PrintCritical

#define PrintDebug(format, ...) PrintDebugFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintInfo(format, ...) PrintInfoFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintWarning(format, ...) PrintWarningFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintError(format, ...) PrintErrorFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintCritical(format, ...) PrintCriticalFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)

typedef struct WorkerNode {
    int workerId;                     
    SOCKET workerSocket;              
    char workerAddress[256];          
    int workerPeerPort;               
    int isConnected;                  
    int isReady;                      
    struct WorkerNode* next;    
    struct WorkerNode* prev;    
} WorkerNode;

typedef struct {
    WorkerNode* head;       
    WorkerNode* current;    
    CRITICAL_SECTION lock;  
    HANDLE notEmpty;        
    HANDLE notFull;         
    int count;
} WorkerList;

typedef struct {
    HANDLE threads[MAX_CLIENTS];        
    SOCKET clientSockets[MAX_CLIENTS];  
    int clientIds[MAX_CLIENTS];         
    BOOL isAvailable[MAX_CLIENTS];      
    CRITICAL_SECTION lock;              
    HANDLE semaphore;                   
    int count;
} ClientThreadPool;

typedef struct {
    HANDLE threads[MAX_WORKERS];        
    SOCKET workerSockets[MAX_WORKERS];  
    BOOL isAvailable[MAX_WORKERS];      
    CRITICAL_SECTION lock;              
    HANDLE semaphore;                   
    int count;
} WorkerThreadPool;

typedef struct {
    SOCKET clientSocket;    
    int clientId;           
    char data[MAX_MESSAGE_SIZE]; 
    uint16_t dataSize;      
    MessageType messageType; 
} ClientRequest;

typedef struct {
    ClientRequest queue[CLIENT_REQUEST_QUEUE_CAPACITY]; 
    int head;                                           
    int tail;                                           
    int count;
    CRITICAL_SECTION lock;                              
    HANDLE notEmpty;                                    
    HANDLE notFull;                                     
} ClientRequestQueue;

typedef struct {
    SOCKET workerSocket;    
    int workerId;           
    char data[MAX_MESSAGE_SIZE]; 
    uint16_t dataSize;      
    MessageType messageType; 
    int clientId;           
} WorkerResponse;

typedef struct {
    WorkerResponse queue[WORKER_RESPONSE_QUEUE_CAPACITY];   
    int head;                                               
    int tail;                                               
    int count;
    CRITICAL_SECTION lock;                                  
    HANDLE notEmpty;                                        
    HANDLE notFull;                                         
} WorkerResponseQueue;

typedef struct {
    CRITICAL_SECTION lock;                      
    HANDLE finishSignal;                        
    int finishFlag;                             
    SOCKET clientListenSocket;                  
    SOCKET workerListenSocket;                  
    struct addrinfo* clientConnectionResultingAddress; 
    struct addrinfo* workerConnectionResultingAddress; 
    WorkerList* workerList;                     
    ClientThreadPool* clientThreadPool;         
    WorkerThreadPool* workerThreadPool;         
    ClientRequestQueue* clientRequestQueue;     
    WorkerResponseQueue* workerResponseQueue;   
} Context;

typedef struct {
    Context* context;
    SOCKET clientSocket;    
    int clientId;           
    int threadIndex;        
} ClientDataReceiverThreadData;

typedef struct {
    Context* context;
    SOCKET workerSocket;    
    int workerId;           
    int threadIndex;        
} WorkerDataReceiverThreadData;

typedef struct {
    Context* context;
    SOCKET workerSocket;    
    int workerId;           
} WorkerHandlerThreadData;

#include "Context.h"
#include "ClientThreadPool.h"
#include "ClientRequestQueue.h"
#include "ClientDataReceiverThread.h"
#include "ClientWorkerResponseDispatcherThread.h"
#include "WorkerList.h"
#include "WorkerClientRequestDispatcherThread.h"
#include "WorkerDataReceiverThread.h"
#include "WorkerThreadPool.h"
#include "WorkerResponseQueue.h"
