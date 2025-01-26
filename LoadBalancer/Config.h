#pragma once

// User-defined constants

#define THREAD_COUNT                            6       // Number of non-dynamic threads

#define INPUT_WAIT_TIME                         10      // ms

#define MAX_CLIENTS                             4       // Number of clients

#define CLIENT_REQUEST_QUEUE_CAPACITY           128     // Number of requests that can be stored in the queue
#define CLIENT_REQUEST_QUEUE_PUT_TIMEOUT        10      // ms
#define CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT       10      // ms
#define CLIENT_REQUEST_QUEUE_EMPTY_SLEEP_TIME   10      // ms
#define CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME    10      // ms

#define CLIENT_THREAD_POOL_ASSIGN_TIMEOUT       10      // ms

#define MAX_WORKERS                             2       // Number of workers

#define WORKER_LIST_ADD_TIMEOUT                 10      // ms
#define WORKER_LIST_REMOVE_TIMEOUT              10      // ms
#define WORKER_LIST_GET_TIMEOUT                 10      // ms
#define WORKER_LIST_EMPTY_SLEEP_TIME            10      // ms
#define WORKER_HEALTH_CHECK_INTERVAL            1000    // ms

#define WORKER_RESPONSE_QUEUE_CAPACITY          128     // Number of responses that can be stored in the queue
#define WORKER_RESPONSE_QUEUE_PUT_TIMEOUT       10      // ms
#define WORKER_RESPONSE_QUEUE_TAKE_TIMEOUT      10      // ms
#define WORKER_RESPONSE_QUEUE_EMPTY_SLEEP_TIME  10      // ms
#define WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME   10      // ms

#define WORKER_THREAD_POOL_ASSIGN_TIMEOUT       10      // ms
