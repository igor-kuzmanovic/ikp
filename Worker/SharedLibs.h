#pragma once

#include "../Lib/SharedConfig.h"

#include <conio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>



#include "../Lib/LoggingLib.h"
#include "../Lib/NetworkLib.h"
#include "../Lib/Protocol.h"
#include "../Lib/SharedConfig.h"
#include "Config.h"

#define LOGGING_NAMESPACE "WR"


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

typedef struct DataNode {
    char* key;                  
    char* value;                
    struct DataNode* next;
} DataNode;

typedef struct {
    DataNode** buckets;
    int bucketCount;
    CRITICAL_SECTION* locks;
} HashTable;

typedef struct PeerConnection {
    int workerId;
    char address[256];
    int port;
    SOCKET socket;
    int isConnected;                 
    int isStale;                     
} PeerConnection;

typedef struct {
    PeerConnection peers[MAX_WORKERS];
    int peerCount;
    CRITICAL_SECTION peerLock;         
    int peerPort;                    
    SOCKET peerListenSocket;           
} PeerManager;

typedef struct {
    uint32_t targetWorkerId;
    char targetAddress[256];
    uint16_t targetPort;
} ExportRequest;

typedef struct {
    ExportRequest requests[MAX_WORKERS];
    int requestCount;
    CRITICAL_SECTION lock;
    HANDLE exportSignal;
    HANDLE finishSignal;
} ExportQueue;

typedef struct {
    CRITICAL_SECTION lock;      
    HANDLE finishSignal;        
    int finishFlag;             
    int workerId;               
    SOCKET connectSocket;       
    HashTable* hashTable;       
    PeerManager* peerManager;   
    ExportQueue* exportQueue;
} Context;

#include "Context.h"
#include "HashTable.h"
#include "PeerManager.h"
#include "ExportQueue.h"


