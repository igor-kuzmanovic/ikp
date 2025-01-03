#pragma once

// User libraries

#include "SharedLibs.h"

// User-defined constants

#define THREAD_COUNT                    3

#define BUFFER_SIZE                     512

#define INPUT_WAIT_TIME                 10

#define SERVER_CONNECT_MAX_RETRIES      10
#define SERVER_CONNECT_RETRY_INTERVAL   1000    // milliseconds
#define SERVER_CONNECT_TIMEOUT          5       // seconds

#define SERVER_ADDRESS                  "127.0.0.1"
#define SERVER_PORT                     5059

#define SERVER_FULL_SLEEP_TIME          1000

#define MESSAGE_SEND_WAIT_TIME          1000
#define MESSAGE_COUNT                   INFINITE
