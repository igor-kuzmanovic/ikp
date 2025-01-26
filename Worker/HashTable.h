#pragma once

// User libraries

#include "SharedLibs.h"

// API

// Functions

int InitializeHashTable(HashTable** table);

int DestroyHashTable(HashTable* table);

int HasHashTable(const HashTable* table, const char* key);

int GetHashTable(const HashTable* table, const char* key, char** value);

int SetHashTable(HashTable* table, const char* key, const char* value);

int DeleteHashTable(const HashTable* table, const char* key);
