#pragma once

#include "SharedLibs.h"

int InitializeHashTable(HashTable** table);
int DestroyHashTable(HashTable* table);
int HasHashTable(const HashTable* table, const char* key);
int GetHashTable(const HashTable* table, const char* key, char** value);
int SetHashTable(HashTable* table, const char* key, const char* value);
int ExportHashTableToPeer(const HashTable* table, PeerManager* peerManager, uint32_t targetWorkerId, int* exportedCount);
int GetHashTableCount(const HashTable* table);
void GetHashTableStats(const HashTable* table, uint32_t* memoryUsage, int* itemCount);


