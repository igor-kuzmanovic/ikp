#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include "conio.h"

// User libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Sender.h"
#include "Receiver.h"

// User-defined constants

#define BUFFER_SIZE 512

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "5059"
