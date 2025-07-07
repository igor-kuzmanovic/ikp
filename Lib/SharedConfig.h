#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define SERVER_ADDRESS                                  "127.0.0.1"
#define SERVER_CLIENT_PORT                              5059
#define SERVER_WORKER_PORT                              5060
#define WORKER_PEER_PORT_BASE                           6000

#define MAX_WORKERS                                     8
#define MAX_CLIENTS                                     8

#define MAX_MESSAGE_SIZE                                1024
#define MAX_ERROR_MSG_SIZE                              1024
#define MAX_KEY_SIZE                                    256
#define MAX_VALUE_SIZE                                  256

#define INPUT_WAIT_TIME                                 10

#define NETWORK_POLLING_DELAY                           100

#define CLIENT_MESSAGE_COUNT                            256
#define CLIENT_MESSAGE_SEND_WAIT_TIME                   100
#define CLIENT_PUT_TO_GET_TRANSITION_DELAY              2000
#define CLIENT_SERVER_FULL_SLEEP_TIME                   1000

#define LB_QUEUE_RETRY_DELAY                            100

#define CLIENT_REQUEST_QUEUE_CAPACITY                   64
#define CLIENT_REQUEST_QUEUE_PUT_TIMEOUT                100
#define CLIENT_REQUEST_QUEUE_TAKE_TIMEOUT               100
#define CLIENT_REQUEST_QUEUE_EMPTY_SLEEP_TIME           100
#define CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME            100

#define CLIENT_THREAD_POOL_ASSIGN_TIMEOUT               100

#define WORKER_LIST_ADD_TIMEOUT                         100
#define WORKER_LIST_REMOVE_TIMEOUT                      100
#define WORKER_LIST_GET_TIMEOUT                         100
#define WORKER_LIST_EMPTY_SLEEP_TIME                    100

#define WORKER_RESPONSE_QUEUE_CAPACITY                  64
#define WORKER_RESPONSE_QUEUE_PUT_TIMEOUT               100
#define WORKER_RESPONSE_QUEUE_TAKE_TIMEOUT              100
#define WORKER_RESPONSE_QUEUE_EMPTY_SLEEP_TIME          100
#define WORKER_RESPONSE_QUEUE_FULL_SLEEP_TIME           100

#define WORKER_THREAD_POOL_ASSIGN_TIMEOUT               100

#define WORKER_STARTUP_DELAY                            100
#define WORKER_HASH_TABLE_BUCKET_COUNT                  32
#define WORKER_DATA_EXPORT_BUCKET_YIELD_SLEEP_TIME      100
#define WORKER_PEER_CONNECTION_RETRY_DELAY              100
