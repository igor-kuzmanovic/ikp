#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// Shared user libraries

#include "SharedConfig.h"

// API

// User-defined constants

#define MIN_KEY_LENGTH                  1
#define MAX_KEY_LENGTH                  120
#define MIN_VALUE_LENGTH                1
#define MAX_VALUE_LENGTH                10

#define PROTOCOL_KEY_VALUE_DELIMITER    ':'

#define SERVER_SHUTDOWN_MESSAGE         "Server is shutting down."
#define SERVER_BUSY_MESSAGE             "Server is busy."

#define WORKER_HEALTH_CHECK_MESSAGE     "Health check."
#define WORKER_OK                       "Ok." // TODO Just for testing

// Structures

typedef enum {
    MSG_UNKNOWN = 0,
    MSG_KEY_VALUE_PAIR = 1,
    MSG_SERVER_SHUTDOWN = 2,
    MSG_SERVER_BUSY = 3,
    MSG_WORKER_HEALTH_CHECK = 4,
    MSG_WORKER_OK = 5,
    MSG_CUSTOM = 6 // For additional messages
} MessageType;

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} KeyValuePair;

typedef struct {
    bool isTrue;
} ServerShutdownMessage;

typedef struct {
    int workerId;
} WorkerHealthCheckMessage;

typedef struct {
    bool isHealthy;
} WorkerHealthResponseMessage;

typedef union {
    KeyValuePair keyValuePair;
    ServerShutdownMessage serverShutdown;
    WorkerHealthCheckMessage healthCheck;
    WorkerHealthResponseMessage healthResponse;
} MessagePayload;

typedef struct {
    MessageType type;
    int length;
    MessagePayload payload;
} Message;

#define BUFFER_SIZE sizeof(Message)

typedef union {
    Message message;
    char buffer[BUFFER_SIZE];
} MessageBuffer;

// Functions

int SerializeMessage(Message* message, char* buffer);

int DeserializeMessage(char* buffer, Message* message);
