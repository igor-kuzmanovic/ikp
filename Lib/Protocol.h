#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>

#include "SharedConfig.h"

// Force 3 byte alignment for structures
#pragma pack(push, 1)
typedef struct {
    uint8_t type;
    uint16_t length;
} MessageHeader;
#pragma pack(pop)

typedef enum {
    MSG_PUT = 1,
    MSG_PUT_RESPONSE = 2,
    MSG_GET = 3,
    MSG_GET_RESPONSE = 4,

    MSG_STORE_REQUEST = 10,
    MSG_STORE_RESPONSE = 11,
    MSG_RETRIEVE_REQUEST = 12,
    MSG_RETRIEVE_RESPONSE = 13,
    MSG_WORKER_REGISTRY_START = 14,
    MSG_WORKER_ENTRY = 15,
    MSG_WORKER_REGISTRY_END = 16,
    MSG_DATA_EXPORT_START = 17,
    MSG_DATA_ENTRY = 18,
    MSG_DATA_EXPORT_END = 19,
    MSG_WORKER_READY = 20,
    MSG_WORKER_NOT_READY = 21,

    MSG_PEER_NOTIFY = 30,

    MSG_SHUTDOWN = 90,
    MSG_ERROR = 91
} MessageType;

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_KEY = 1,
    ERR_INVALID_VALUE = 2,
    ERR_KEY_NOT_FOUND = 3,
    ERR_SERVER_BUSY = 4,
    ERR_NETWORK_ERROR = 5,
    ERR_PROTOCOL_ERROR = 6
} ErrorCode;

int ProtocolSend(SOCKET socket, MessageType type, const void* data, uint16_t dataSize);
int ProtocolReceive(SOCKET socket, MessageType* type, void* buffer, uint16_t bufferSize, uint16_t* actualSize);

int SendPut(SOCKET socket, const char* key, const char* value);
int SendPutResponse(SOCKET socket, ErrorCode result, const char* key);
int SendGet(SOCKET socket, const char* key);
int SendGetResponse(SOCKET socket, ErrorCode result, const char* key, const char* value);
int SendStoreRequest(SOCKET socket, uint32_t clientId, const char* key, const char* value);
int SendStoreResponse(SOCKET socket, ErrorCode result, uint32_t clientId, const char* key);
int SendRetrieveRequest(SOCKET socket, uint32_t clientId, const char* key);
int SendRetrieveResponse(SOCKET socket, ErrorCode result, uint32_t clientId, const char* key, const char* value);
int SendWorkerReady(SOCKET socket, uint32_t workerId, uint16_t peerPort);
int SendWorkerNotReady(SOCKET socket, uint32_t workerId);
int SendWorkerRegistryStart(SOCKET socket, uint32_t totalWorkers);
int SendWorkerEntry(SOCKET socket, uint32_t workerId, const char* address, uint16_t port, uint8_t shouldExportData);
int SendWorkerRegistryEnd(SOCKET socket);
int SendDataExportStart(SOCKET socket, uint32_t totalEntries);
int SendDataEntry(SOCKET socket, const char* key, const char* value);
int SendDataExportEnd(SOCKET socket);
int SendPeerNotify(SOCKET socket, const char* key, const char* value);
int SendError(SOCKET socket, ErrorCode error, const char* message);
int SendShutdown(SOCKET socket);

int ReceivePut(const char* buffer, uint16_t bufferSize, char* key, char* value);
int ReceivePutResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, char* key);
int ReceiveGet(const char* buffer, uint16_t bufferSize, char* key);
int ReceiveGetResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, char* key, char* value);
int ReceiveStoreRequest(const char* buffer, uint16_t bufferSize, uint32_t* clientId, char* key, char* value);
int ReceiveStoreResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, uint32_t* clientId, char* key);
int ReceiveRetrieveRequest(const char* buffer, uint16_t bufferSize, uint32_t* clientId, char* key);
int ReceiveRetrieveResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, uint32_t* clientId, char* key, char* value);
int ReceiveWorkerReady(const char* buffer, uint16_t bufferSize, uint32_t* workerId, uint16_t* peerPort);
int ReceiveWorkerNotReady(const char* buffer, uint16_t bufferSize, uint32_t* workerId);
int ReceiveWorkerRegistryStart(const char* buffer, uint16_t bufferSize, uint32_t* totalWorkers);
int ReceiveWorkerEntry(const char* buffer, uint16_t bufferSize, uint32_t* workerId, char* address, uint16_t* port, uint8_t* shouldExportData);
int ReceiveDataExportStart(const char* buffer, uint16_t bufferSize, uint32_t* totalEntries);
int ReceiveDataEntry(const char* buffer, uint16_t bufferSize, char* key, char* value);
int ReceivePeerNotify(const char* buffer, uint16_t bufferSize, char* key, char* value);
int ReceiveError(const char* buffer, uint16_t bufferSize, ErrorCode* errorCode, char* message);

const char* GetMessageTypeName(MessageType type);
const char* GetErrorCodeName(ErrorCode code);

int ValidateKey(const char* key);
int ValidateValue(const char* value);

int WriteUInt8(void* buffer, uint8_t value);
int WriteUInt16(void* buffer, uint16_t value);
int WriteUInt32(void* buffer, uint32_t value);
int WriteString(void* buffer, const char* str);

uint8_t ReadUInt8(const void* buffer);
uint16_t ReadUInt16(const void* buffer);
uint32_t ReadUInt32(const void* buffer);
int ReadString(const void* buffer, char* str, size_t maxLen);
