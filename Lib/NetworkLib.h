#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#define _WINSOCK_DEPRECATED_NO_WARNINGS	// Disable warnings for networking-related APIs

// System libraries

#include <stdlib.h>
#include <stdio.h>

// Networking libraries

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Need to link with Ws2_32.lib

// User libraries

#include "LoggingLib.h"

// API

// Functions

// Initializes Winsock
int InitializeWindowsSockets();
