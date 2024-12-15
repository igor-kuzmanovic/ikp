#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include "conio.h"

// User-defined constants

#define BUFFER_SIZE 512

#define SERVER_PORT "5059"

// User libraries

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Context.h"
#include "ClientHandlerThread.h"
#include "InputHandlerThread.h"
