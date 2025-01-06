#pragma once

// User libraries

#include "SharedLibs.h"

// User-defined constants

#define THREAD_COUNT                    3           // Number of non-dynamic threads

#define INPUT_WAIT_TIME                 10          // ms

#define SERVER_CONNECT_MAX_RETRIES      10
#define SERVER_CONNECT_RETRY_INTERVAL   1000        // ms
#define SERVER_CONNECT_TIMEOUT          5           // s

#define SERVER_FULL_SLEEP_TIME          1000        // ms

#define MESSAGE_SEND_WAIT_TIME          1000        // ms
#define MESSAGE_COUNT                   INFINITE    // INFINITE or a positive integer
