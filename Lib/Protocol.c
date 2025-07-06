#include "Protocol.h"
#include "LoggingLib.h"
#include "NetworkLib.h"

#define PROTOCOL_NAMESPACE "PROTO"
#define PrintDebug(format, ...) PrintDebugFunc(PROTOCOL_NAMESPACE, format, ##__VA_ARGS__)
#define PrintInfo(format, ...) PrintInfoFunc(PROTOCOL_NAMESPACE, format, ##__VA_ARGS__)
#define PrintWarning(format, ...) PrintWarningFunc(PROTOCOL_NAMESPACE, format, ##__VA_ARGS__)
#define PrintError(format, ...) PrintErrorFunc(PROTOCOL_NAMESPACE, format, ##__VA_ARGS__)
#define PrintCritical(format, ...) PrintCriticalFunc(PROTOCOL_NAMESPACE, format, ##__VA_ARGS__)

static int ProtocolSendBuffer(SOCKET socket, const void* buffer, size_t size);
static int ProtocolReceiveBuffer(SOCKET socket, void* buffer, size_t size);

int ProtocolSend(SOCKET socket, MessageType type, const void* data, uint16_t dataSize) {
    if (socket == INVALID_SOCKET) {
        PrintError("ProtocolSend: Invalid socket");
        return -1;
    }

    if (dataSize > MAX_MESSAGE_SIZE) {
        PrintError("ProtocolSend: Message too large (%u bytes)", dataSize);
        return -1;
    }

    MessageHeader header;
    header.type = (uint8_t)type;
    header.length = htons(dataSize);
    int sendResult = ProtocolSendBuffer(socket, &header, sizeof(header));
    if (sendResult != 0) {
        if (sendResult == -2) {
            PrintInfo("ProtocolSend: Connection closed (error code %d)", sendResult);
        } else {
            PrintError("ProtocolSend: Failed to send header (error code %d)", sendResult);
        }
        return sendResult;
    }

    if (dataSize > 0 && data != NULL) {
        sendResult = ProtocolSendBuffer(socket, data, dataSize);
        if (sendResult != 0) {
            if (sendResult == -2) {
                PrintInfo("ProtocolSend: Connection closed (error code %d)", sendResult);
            } else {
                PrintError("ProtocolSend: Failed to send payload (error code %d)", sendResult);
            }
            return sendResult;
        }
    }

    return sizeof(MessageHeader) + dataSize;
}

int ProtocolReceive(SOCKET socket, MessageType* type, void* buffer, uint16_t bufferSize, uint16_t* actualSize) {
    if (socket == INVALID_SOCKET) {
        PrintError("ProtocolReceive: Invalid socket");
        return -1;
    }

    MessageHeader header;
    int recvResult = ProtocolReceiveBuffer(socket, &header, sizeof(header));

    if (recvResult != 0) {
        if (recvResult == -2) {
            PrintInfo("ProtocolReceive: Connection closed (error code %d)", recvResult);
        } else {
            PrintError("ProtocolReceive: Failed to receive header (error code %d)", recvResult);
        }
        return recvResult;
    }

    if (header.type == 0) {
        PrintError("ProtocolReceive: Invalid message type %u in header", header.type);
        return -4;
    }

    *type = (MessageType)header.type;
    *actualSize = ntohs(header.length);

    PrintDebug("Received header: type=%u, length=%u", header.type, *actualSize);

    if (*actualSize > MAX_MESSAGE_SIZE * 4) {
        PrintError("ProtocolReceive: Suspicious message size %u, possible protocol corruption", *actualSize);
        return -4;
    }

    if (*actualSize > 0) {
        if (*actualSize > bufferSize) {
            PrintError("ProtocolReceive: Buffer too small (%u needed, %u available)", *actualSize, bufferSize);
            return -1;
        }

        recvResult = ProtocolReceiveBuffer(socket, buffer, *actualSize);
        if (recvResult != 0) {
            if (recvResult == -2) {
                PrintInfo("ProtocolReceive: Connection closed (error code %d)", recvResult);
            } else {
                PrintError("ProtocolReceive: Failed to receive payload (error code %d)", recvResult);
            }
            return recvResult;
        }
    }

    return 0;
}

int SendPut(SOCKET socket, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    uint16_t valueLen = (uint16_t)strlen(value);
    offset += WriteUInt16(buffer + offset, valueLen);
    offset += WriteString(buffer + offset, value);

    return ProtocolSend(socket, MSG_PUT, buffer, offset);
}

int SendPutResponse(SOCKET socket, ErrorCode result, const char* key) {
    if (!ValidateKey(key)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt8(buffer + offset, (uint8_t)result);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    return ProtocolSend(socket, MSG_PUT_RESPONSE, buffer, offset);
}

int SendGet(SOCKET socket, const char* key) {
    if (!ValidateKey(key)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    return ProtocolSend(socket, MSG_GET, buffer, offset);
}

int SendGetResponse(SOCKET socket, ErrorCode result, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt8(buffer + offset, (uint8_t)result);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    if (result == ERR_NONE && value != NULL) {
        uint16_t valueLen = (uint16_t)strlen(value);
        offset += WriteUInt16(buffer + offset, valueLen);
        offset += WriteString(buffer + offset, value);
    } else {
        offset += WriteUInt16(buffer + offset, 0);
    }

    return ProtocolSend(socket, MSG_GET_RESPONSE, buffer, offset);
}

int SendStoreRequest(SOCKET socket, uint32_t clientId, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, clientId);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    uint16_t valueLen = (uint16_t)strlen(value);
    offset += WriteUInt16(buffer + offset, valueLen);
    offset += WriteString(buffer + offset, value);

    return ProtocolSend(socket, MSG_STORE_REQUEST, buffer, offset);
}

int SendStoreResponse(SOCKET socket, ErrorCode result, uint32_t clientId, const char* key) {
    if (!ValidateKey(key)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt8(buffer + offset, (uint8_t)result);

    offset += WriteUInt32(buffer + offset, clientId);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    return ProtocolSend(socket, MSG_STORE_RESPONSE, buffer, offset);
}

int SendRetrieveRequest(SOCKET socket, uint32_t clientId, const char* key) {
    if (!ValidateKey(key)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, clientId);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    return ProtocolSend(socket, MSG_RETRIEVE_REQUEST, buffer, offset);
}

int SendRetrieveResponse(SOCKET socket, ErrorCode result, uint32_t clientId, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt8(buffer + offset, (uint8_t)result);

    offset += WriteUInt32(buffer + offset, clientId);

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    if (result == ERR_NONE && value != NULL) {
        uint16_t valueLen = (uint16_t)strlen(value);
        offset += WriteUInt16(buffer + offset, valueLen);
        offset += WriteString(buffer + offset, value);
    } else {
        offset += WriteUInt16(buffer + offset, 0);
    }

    return ProtocolSend(socket, MSG_RETRIEVE_RESPONSE, buffer, offset);
}

int SendWorkerReady(SOCKET socket, uint32_t workerId, uint16_t peerPort) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, workerId);
    offset += WriteUInt16(buffer + offset, peerPort);

    return ProtocolSend(socket, MSG_WORKER_READY, buffer, offset);
}

int SendWorkerNotReady(SOCKET socket, uint32_t workerId) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, workerId);

    return ProtocolSend(socket, MSG_WORKER_NOT_READY, buffer, offset);
}

int SendWorkerRegistryStart(SOCKET socket, uint32_t totalWorkers) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, totalWorkers);

    return ProtocolSend(socket, MSG_WORKER_REGISTRY_START, buffer, offset);
}

int SendWorkerEntry(SOCKET socket, uint32_t workerId, const char* address, uint16_t port, uint8_t shouldExportData) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, workerId);

    uint16_t addrLen = (uint16_t)strlen(address);
    if (addrLen > 255) addrLen = 255;
    offset += WriteUInt16(buffer + offset, addrLen);
    memcpy(buffer + offset, address, addrLen);
    offset += addrLen;

    offset += WriteUInt16(buffer + offset, port);
    offset += WriteUInt8(buffer + offset, shouldExportData);

    return ProtocolSend(socket, MSG_WORKER_ENTRY, buffer, offset);
}

int SendWorkerRegistryEnd(SOCKET socket) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    return ProtocolSend(socket, MSG_WORKER_REGISTRY_END, buffer, offset);
}

int SendDataExportStart(SOCKET socket, uint32_t totalEntries) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt32(buffer + offset, totalEntries);

    return ProtocolSend(socket, MSG_DATA_EXPORT_START, buffer, offset);
}

int SendDataEntry(SOCKET socket, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    uint16_t valueLen = (uint16_t)strlen(value);
    offset += WriteUInt16(buffer + offset, valueLen);
    offset += WriteString(buffer + offset, value);

    return ProtocolSend(socket, MSG_DATA_ENTRY, buffer, offset);
}

int SendDataExportEnd(SOCKET socket) {
    return ProtocolSend(socket, MSG_DATA_EXPORT_END, NULL, 0);
}

int SendPeerNotify(SOCKET socket, const char* key, const char* value) {
    if (!ValidateKey(key) || !ValidateValue(value)) {
        return -1;
    }

    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    uint16_t keyLen = (uint16_t)strlen(key);
    offset += WriteUInt16(buffer + offset, keyLen);
    offset += WriteString(buffer + offset, key);

    uint16_t valueLen = (uint16_t)strlen(value);
    offset += WriteUInt16(buffer + offset, valueLen);
    offset += WriteString(buffer + offset, value);

    return ProtocolSend(socket, MSG_PEER_NOTIFY, buffer, offset);
}

int SendError(SOCKET socket, ErrorCode error, const char* message) {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    uint16_t offset = 0;

    offset += WriteUInt8(buffer + offset, (uint8_t)error);

    if (message != NULL) {
        uint16_t msgLen = (uint16_t)strlen(message);
        if (msgLen > MAX_ERROR_MSG_SIZE) {
            msgLen = MAX_ERROR_MSG_SIZE;
        }
        offset += WriteUInt16(buffer + offset, msgLen);
        memcpy(buffer + offset, message, msgLen);
        offset += msgLen;
    } else {
        offset += WriteUInt16(buffer + offset, 0);
    }

    return ProtocolSend(socket, MSG_ERROR, buffer, offset);
}

int SendShutdown(SOCKET socket) {
    return ProtocolSend(socket, MSG_SHUTDOWN, NULL, 0);
}

const char* GetMessageTypeName(MessageType type) {
    switch (type) {
    case MSG_PUT: return "PUT";
    case MSG_PUT_RESPONSE: return "PUT_RESPONSE";
    case MSG_GET: return "GET";
    case MSG_GET_RESPONSE: return "GET_RESPONSE";
    case MSG_STORE_REQUEST: return "STORE_REQUEST";
    case MSG_STORE_RESPONSE: return "STORE_RESPONSE";
    case MSG_RETRIEVE_REQUEST: return "RETRIEVE_REQUEST";
    case MSG_RETRIEVE_RESPONSE: return "RETRIEVE_RESPONSE";
    case MSG_WORKER_READY: return "WORKER_READY";
    case MSG_WORKER_NOT_READY: return "WORKER_NOT_READY";
    case MSG_WORKER_REGISTRY_START: return "WORKER_REGISTRY_START";
    case MSG_WORKER_ENTRY: return "WORKER_ENTRY";
    case MSG_WORKER_REGISTRY_END: return "WORKER_REGISTRY_END";
    case MSG_DATA_EXPORT_START: return "DATA_EXPORT_START";
    case MSG_DATA_ENTRY: return "DATA_ENTRY";
    case MSG_DATA_EXPORT_END: return "DATA_EXPORT_END";
    case MSG_PEER_NOTIFY: return "PEER_NOTIFY";
    case MSG_SHUTDOWN: return "SHUTDOWN";
    case MSG_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

const char* GetErrorCodeName(ErrorCode code) {
    switch (code) {
    case ERR_NONE: return "NONE";
    case ERR_INVALID_KEY: return "INVALID_KEY";
    case ERR_INVALID_VALUE: return "INVALID_VALUE";
    case ERR_KEY_NOT_FOUND: return "KEY_NOT_FOUND";
    case ERR_SERVER_BUSY: return "SERVER_BUSY";
    case ERR_NETWORK_ERROR: return "NETWORK_ERROR";
    case ERR_PROTOCOL_ERROR: return "PROTOCOL_ERROR";
    default: return "UNKNOWN";
    }
}

int ValidateKey(const char* key) {
    if (key == NULL || strlen(key) == 0 || strlen(key) > MAX_KEY_SIZE) {
        return 0;
    }
    return 1;
}

int ValidateValue(const char* value) {
    if (value == NULL || strlen(value) > MAX_VALUE_SIZE) {
        return 0;
    }
    return 1;
}

static int ProtocolSendBuffer(SOCKET socket, const void* buffer, size_t size) {
    const char* data = (const char*)buffer;

    int result = SendBuffer(socket, data, (int)size);

    if (result < 0) {
        switch (result) {
        case -2:
            return -2;
        case -3:
            PrintError("ProtocolSendBuffer: Network or host unreachable");
            return -3;
        default:
            PrintError("ProtocolSendBuffer: Send operation failed with error code %d", result);
            return -1;
        }
    }

    return 0;
}

static int ProtocolReceiveBuffer(SOCKET socket, void* buffer, size_t size) {
    char* data = (char*)buffer;

    int result = ReceiveBuffer(socket, data, (int)size);

    if (result < 0) {
        switch (result) {
        case -2:
            return -2;
        case -3:
            PrintError("ProtocolReceiveBuffer: Network or host unreachable");
            return -3;
        default:
            PrintError("ProtocolReceiveBuffer: Receive operation failed with error code %d", result);
            return -1;
        }
    }

    return 0;
}

int WriteUInt32(void* buffer, uint32_t value) {
    uint32_t netValue = htonl(value);
    memcpy(buffer, &netValue, sizeof(uint32_t));
    return sizeof(uint32_t);
}

int WriteUInt16(void* buffer, uint16_t value) {
    uint16_t netValue = htons(value);
    memcpy(buffer, &netValue, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int WriteUInt8(void* buffer, uint8_t value) {
    memcpy(buffer, &value, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int WriteString(void* buffer, const char* str) {
    size_t len = strlen(str);
    memcpy(buffer, str, len);
    return (int)len;
}

uint32_t ReadUInt32(const void* buffer) {
    uint32_t netValue;
    memcpy(&netValue, buffer, sizeof(uint32_t));
    return ntohl(netValue);
}

uint16_t ReadUInt16(const void* buffer) {
    uint16_t netValue;
    memcpy(&netValue, buffer, sizeof(uint16_t));
    return ntohs(netValue);
}

uint8_t ReadUInt8(const void* buffer) {
    uint8_t value;
    memcpy(&value, buffer, sizeof(uint8_t));
    return value;
}

int ReadString(const void* buffer, char* str, size_t maxLen) {
    if (maxLen == 0) {
        return -1;
    }

    size_t len = maxLen - 1;
    memcpy(str, buffer, len);
    str[len] = '\0';
    return 0;
}

int ReceivePut(const char* buffer, uint16_t bufferSize, char* key, char* value) {
    uint16_t offset = 0;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
    ReadString(buffer + offset, value, valueLen + 1);

    return 0;
}

int ReceivePutResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, char* key) {
    uint16_t offset = 0;

    if (offset + 1 > bufferSize) return -1;
    *result = (ErrorCode)ReadUInt8(buffer + offset);
    offset += 1;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);

    return 0;
}

int ReceiveGet(const char* buffer, uint16_t bufferSize, char* key) {
    uint16_t offset = 0;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);

    return 0;
}

int ReceiveGetResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, char* key, char* value) {
    uint16_t offset = 0;

    if (offset + 1 > bufferSize) return -1;
    *result = (ErrorCode)ReadUInt8(buffer + offset);
    offset += 1;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > 0) {
        if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
        ReadString(buffer + offset, value, valueLen + 1);
    } else {
        value[0] = '\0';
    }

    return 0;
}

int ReceiveStoreResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, uint32_t* clientId, char* key) {
    uint16_t offset = 0;

    if (offset + 1 > bufferSize) return -1;
    *result = (ErrorCode)ReadUInt8(buffer + offset);
    offset += 1;

    if (offset + 4 > bufferSize) return -1;
    *clientId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);

    return 0;
}

int ReceiveRetrieveResponse(const char* buffer, uint16_t bufferSize, ErrorCode* result, uint32_t* clientId, char* key, char* value) {
    uint16_t offset = 0;

    if (offset + 1 > bufferSize) return -1;
    *result = (ErrorCode)ReadUInt8(buffer + offset);
    offset += 1;

    if (offset + 4 > bufferSize) return -1;
    *clientId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > 0) {
        if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
        ReadString(buffer + offset, value, valueLen + 1);
    } else {
        value[0] = '\0';
    }

    return 0;
}

int ReceiveDataEntry(const char* buffer, uint16_t bufferSize, char* key, char* value) {
    uint16_t offset = 0;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
    ReadString(buffer + offset, value, valueLen + 1);

    return 0;
}

int ReceivePeerNotify(const char* buffer, uint16_t bufferSize, char* key, char* value) {
    if (!buffer || !key || !value) {
        return -1;
    }

    uint16_t offset = 0;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
    ReadString(buffer + offset, value, valueLen + 1);

    return 0;
}

int ReceiveWorkerRegistryStart(const char* buffer, uint16_t bufferSize, uint32_t* totalWorkers) {
    if (!buffer || !totalWorkers) {
        return -1;
    }

    if (bufferSize < 4) return -1;

    *totalWorkers = ReadUInt32(buffer);
    return 0;
}

int ReceiveError(const char* buffer, uint16_t bufferSize, ErrorCode* errorCode, char* message) {
    if (!buffer || !errorCode || !message) {
        return -1;
    }

    uint16_t offset = 0;

    if (offset >= bufferSize) return -1;
    *errorCode = (ErrorCode)ReadUInt8(buffer + offset);
    offset += 1;

    if (offset + 2 > bufferSize) return -1;
    uint16_t messageLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (messageLen > 0) {
        if (offset + messageLen > bufferSize) return -1;
        if (messageLen >= 256) messageLen = 255;
        ReadString(buffer + offset, message, messageLen + 1);
    } else {
        message[0] = '\0';
    }

    return 0;
}

int ReceiveDataExportStart(const char* buffer, uint16_t bufferSize, uint32_t* totalEntries) {
    if (!buffer || !totalEntries) {
        return -1;
    }

    if (bufferSize < 4) return -1;

    *totalEntries = ReadUInt32(buffer);
    return 0;
}

int ReceiveWorkerReady(const char* buffer, uint16_t bufferSize, uint32_t* workerId, uint16_t* peerPort) {
    uint16_t offset = 0;

    if (offset + 4 > bufferSize) return -1;
    *workerId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    *peerPort = ReadUInt16(buffer + offset);

    return 0;
}

int ReceiveWorkerNotReady(const char* buffer, uint16_t bufferSize, uint32_t* workerId) {
    uint16_t offset = 0;

    if (offset + 4 > bufferSize) return -1;
    *workerId = ReadUInt32(buffer + offset);

    return 0;
}

int ReceiveStoreRequest(const char* buffer, uint16_t bufferSize, uint32_t* clientId, char* key, char* value) {
    uint16_t offset = 0;

    if (offset + 4 > bufferSize) return -1;
    *clientId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);
    offset += keyLen;

    if (offset + 2 > bufferSize) return -1;
    uint16_t valueLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (valueLen > MAX_VALUE_SIZE || offset + valueLen > bufferSize) return -1;
    ReadString(buffer + offset, value, valueLen + 1);

    return 0;
}

int ReceiveRetrieveRequest(const char* buffer, uint16_t bufferSize, uint32_t* clientId, char* key) {
    uint16_t offset = 0;

    if (offset + 4 > bufferSize) return -1;
    *clientId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    uint16_t keyLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (keyLen > MAX_KEY_SIZE || offset + keyLen > bufferSize) return -1;
    ReadString(buffer + offset, key, keyLen + 1);

    return 0;
}

int ReceiveWorkerEntry(const char* buffer, uint16_t bufferSize, uint32_t* workerId, char* address, uint16_t* port, uint8_t* shouldExportData) {
    uint16_t offset = 0;

    if (offset + 4 > bufferSize) return -1;
    *workerId = ReadUInt32(buffer + offset);
    offset += 4;

    if (offset + 2 > bufferSize) return -1;
    uint16_t addrLen = ReadUInt16(buffer + offset);
    offset += 2;

    if (addrLen >= 256 || offset + addrLen > bufferSize) return -1;
    ReadString(buffer + offset, address, addrLen + 1);
    offset += addrLen;

    if (offset + 2 > bufferSize) return -1;
    *port = ReadUInt16(buffer + offset);
    offset += 2;

    if (offset + 1 > bufferSize) return -1;
    *shouldExportData = ReadUInt8(buffer + offset);

    return 0;
}

