#pragma once

// User-defined constants

#define THREAD_COUNT                            4           // Number of non-dynamic threads

#define INPUT_WAIT_TIME                         10          // ms

#define MAX_CLIENTS                             2           // Number of clients
#define MAX_WORKERS                             1           // Number of workers

#define CLIENT_THREAD_POOL_SIZE                 MAX_CLIENTS // Number of threads in the client thread pool

#define CLIENT_REQUEST_QUEUE_CAPACITY           4
#define CLIENT_REQUEST_QUEUE_PUT_TIMEOUT        10          // ms
#define CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT       10          // ms
#define CLIENT_REQUEST_QUEUE_EMPTY_SLEEP_TIME   10          // ms
#define CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME    10          // ms

#define CLIENT_THREAD_POOL_ASSIGN_TIMEOUT       10          // ms

#define WORKER_LIST_GET_TIMEOUT                 10          // ms
#define WORKER_LIST_EMPTY_SLEEP_TIME            10          // ms
