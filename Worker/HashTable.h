#pragma once

#include "SharedLibs.h"

int InitializeHashTable(HashTable** table);
int DestroyHashTable(HashTable* table);
int HasHashTable(const HashTable* table, const char* key);
int GetHashTable(const HashTable* table, const char* key, char** value);
int SetHashTable(HashTable* table, const char* key, const char* value);
int DeleteHashTable(const HashTable* table, const char* key);
int ExportHashTableToPeer(const HashTable* table, PeerManager* peerManager, uint32_t targetWorkerId, int* exportedCount);
int ImportHashTableData(HashTable* table, const char* importData, int dataSize);
int GetHashTableCount(const HashTable* table);
uint32_t GetHashTableMemoryUsage(const HashTable* table);
void GetHashTableStats(const HashTable* table, uint32_t* memoryUsage, int* itemCount);


