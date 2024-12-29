#pragma once

// User-defined constants

#define THREAD_COUNT 4

#define BUFFER_SIZE 512

#define SERVER_CLIENT_PORT "5059"
#define SERVER_WORKER_PORT "5060"

#define INPUT_WAIT_TIME 10 // ms

#define MAX_CLIENTS 2
#define MAX_WORKERS 1

#define CLIENT_THREAD_POOL_SIZE 2

#define CLIENT_REQUEST_QUEUE_CAPACITY 512
#define CLIENT_REQUEST_QUEUE_PUT_TIMEOUT 10 // ms
#define CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT 10 // ms
#define CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME 10 // ms

#define CLIENT_THREAD_POOL_ASSIGN_TIMEOUT 10 // ms

#define WORKER_LIST_GET_TIMEOUT 10 // ms
