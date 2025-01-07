#pragma once

// User-defined constants

#define MAX_KEY_LENGTH              256
#define MAX_VALUE_LENGTH            768
#define BUFFER_SIZE                 (MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 1) // +1 for delimiter

#define SERVER_ADDRESS              "127.0.0.1"
#define SERVER_CLIENT_PORT          5059
#define SERVER_WORKER_PORT          5060

#define SERVER_SHUTDOWN_MESSAGE     "Server is shutting down."
#define SERVER_BUSY_MESSAGE         "Server is busy."

#define WORKER_HEALTH_CHECK_MESSAGE "Health check."