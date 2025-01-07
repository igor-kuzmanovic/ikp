#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <stdio.h>
#include <string.h>

// Shared user libraries

#include "SharedConfig.h"

// API

// Structures

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} KeyValuePair;

// Functions

// Serializes the struct into a buffer
int SerializeKVPair(const KeyValuePair* kvp, char* buffer);

// Deserializes the struct from a buffer
int DeserializeKVPair(const char* buffer, KeyValuePair* kvp);
