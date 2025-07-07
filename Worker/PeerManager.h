#pragma once

#include "SharedLibs.h"

int InitializePeerManager(PeerManager* peerManager, int peerPort);
int DestroyPeerManager(PeerManager* peerManager);
int BroadcastData(PeerManager* peerManager, const char* key, const char* value);
int AddPeer(PeerManager* peerManager, int workerId, const char* address, int port);
int FinalizePeerRegistryUpdate(PeerManager* peerManager, int* activeCount, int* removedCount);
int RemoveDisconnectedPeer(PeerManager* peerManager, int workerId);
SOCKET GetPeerConnection(PeerManager* peerManager, int targetWorkerId);
