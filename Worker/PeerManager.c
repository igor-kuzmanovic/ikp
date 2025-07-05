#include "PeerManager.h"

int InitializePeerManager(PeerManager* peerManager, int peerPort) {
    if (!peerManager) {
        PrintError("InitializePeerManager: peerManager is NULL");
        return -1;
    }

    peerManager->peerCount = 0;
    peerManager->peerPort = peerPort;
    peerManager->peerListenSocket = INVALID_SOCKET;

    InitializeCriticalSection(&peerManager->peerLock);

    for (int i = 0; i < MAX_WORKERS; i++) {
        peerManager->peers[i].workerId = -1;
        peerManager->peers[i].socket = INVALID_SOCKET;
        peerManager->peers[i].isConnected = 0;
        peerManager->peers[i].isStale = 0;
        memset(peerManager->peers[i].address, 0, sizeof(peerManager->peers[i].address));
        peerManager->peers[i].port = 0;
    }

    PrintDebug("Peer manager initialized on port %d", peerPort);
    return 0;
}

int DestroyPeerManager(PeerManager* peerManager) {
    if (!peerManager) {
        return -1;
    }

    EnterCriticalSection(&peerManager->peerLock);

    for (int i = 0; i < MAX_WORKERS; i++) {
        if (peerManager->peers[i].socket != INVALID_SOCKET) {
            SafeCloseSocket(&peerManager->peers[i].socket);
        }
        peerManager->peers[i].isConnected = 0;
    }

    if (peerManager->peerListenSocket != INVALID_SOCKET) {
        SafeCloseSocket(&peerManager->peerListenSocket);
    }

    peerManager->peerCount = 0;

    LeaveCriticalSection(&peerManager->peerLock);
    DeleteCriticalSection(&peerManager->peerLock);

    PrintDebug("Peer manager destroyed");
    return 0;
}

int BroadcastDataToPeers(PeerManager* peerManager, const char* key, const char* value) {
    if (!peerManager || !key || !value) {
        return -1;
    }

    int successCount = 0;
    int peersToRemove[MAX_WORKERS];
    int removeCount = 0;
    
    EnterCriticalSection(&peerManager->peerLock);

    for (int i = 0; i < MAX_WORKERS; i++) {
        if (peerManager->peers[i].workerId != -1 && peerManager->peers[i].isConnected) {
            int result = SendPeerNotify(peerManager->peers[i].socket, key, value);
            if (result > 0) {
                successCount++;
            } else if (result == -2 || result == -3 || result == -1) {
                PrintInfo("Connection to peer %d lost during broadcast", peerManager->peers[i].workerId);
                peersToRemove[removeCount++] = peerManager->peers[i].workerId;
            }
        }
    }

    LeaveCriticalSection(&peerManager->peerLock);
    
    for (int i = 0; i < removeCount; i++) {
        RemoveDisconnectedPeer(peerManager, peersToRemove[i]);
    }

    PrintDebug("Broadcasted data (%s:%s) to %d peers", key, value, successCount);
    return successCount;
}

int AddSinglePeerWorker(PeerManager* peerManager, int workerId, const char* address, int port) {
    if (peerManager == NULL || address == NULL) {
        return -1;
    }

    if (port == peerManager->peerPort) {
        return 0;
    }

    EnterCriticalSection(&peerManager->peerLock);
    
    for (int i = 0; i < peerManager->peerCount; i++) {
        if (peerManager->peers[i].workerId == workerId) {
            strncpy_s(peerManager->peers[i].address, sizeof(peerManager->peers[i].address), address, _TRUNCATE);
            peerManager->peers[i].port = port;
            LeaveCriticalSection(&peerManager->peerLock);
            return 0;
        }
    }
    
    if (peerManager->peerCount >= MAX_WORKERS) {
        LeaveCriticalSection(&peerManager->peerLock);
        return -1;
    }

    PeerConnection* peer = &peerManager->peers[peerManager->peerCount];
    peer->workerId = workerId;
    strncpy_s(peer->address, sizeof(peer->address), address, _TRUNCATE);
    peer->port = port;
    peer->socket = INVALID_SOCKET;
    peer->isConnected = 0;
    
    peerManager->peerCount++;
    
    PrintInfo("New worker %d added to peer registry, connecting...", workerId);
    
    SOCKET newSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (newSocket != INVALID_SOCKET) {
        struct sockaddr_in targetAddr;
        targetAddr.sin_family = AF_INET;
        targetAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, address, &targetAddr.sin_addr) > 0) {
            int connectResult = SafeConnect(newSocket, (struct sockaddr*)&targetAddr, sizeof(targetAddr), 10);
            if (connectResult == 0) {
                peer->socket = newSocket;
                peer->isConnected = 1;
                PrintInfo("Successfully connected to new worker %d at %s:%d", workerId, address, port);
            } else {
                SafeCloseSocket(&newSocket);
                PrintWarning("Failed to connect to new worker %d at %s:%d (error: %d)", workerId, address, port, connectResult);
            }
        } else {
            SafeCloseSocket(&newSocket);
            PrintError("Invalid address for new worker %d: %s", workerId, address);
        }
    } else {
        PrintError("Failed to create socket for connection to new worker %d", workerId);
    }
    
    LeaveCriticalSection(&peerManager->peerLock);
    return 1;
}

int FinalizePeerRegistryUpdate(PeerManager* peerManager, int* activeCount, int* removedCount) {
    if (peerManager == NULL || activeCount == NULL || removedCount == NULL) {
        return -1;
    }

    *activeCount = peerManager->peerCount;
    *removedCount = 0;

    return 0;
}

int RemoveDisconnectedPeer(PeerManager* peerManager, int workerId) {
    if (!peerManager) {
        return -1;
    }

    EnterCriticalSection(&peerManager->peerLock);

    for (int i = 0; i < peerManager->peerCount; i++) {
        if (peerManager->peers[i].workerId == workerId) {
            if (peerManager->peers[i].socket != INVALID_SOCKET) {
                SafeCloseSocket(&peerManager->peers[i].socket);
            }
            
            for (int j = i; j < peerManager->peerCount - 1; j++) {
                peerManager->peers[j] = peerManager->peers[j + 1];
            }
            
            peerManager->peers[peerManager->peerCount - 1].workerId = -1;
            peerManager->peers[peerManager->peerCount - 1].socket = INVALID_SOCKET;
            peerManager->peers[peerManager->peerCount - 1].isConnected = 0;
            peerManager->peerCount--;
            
            LeaveCriticalSection(&peerManager->peerLock);
            PrintInfo("Removed disconnected peer %d from registry", workerId);
            return 0;
        }
    }

    LeaveCriticalSection(&peerManager->peerLock);
    return -1;
}

SOCKET GetPeerConnection(PeerManager* peerManager, int targetWorkerId) {
    if (!peerManager) {
        PrintError("GetPeerConnection: peerManager is NULL");
        return INVALID_SOCKET;
    }

    EnterCriticalSection(&peerManager->peerLock);

    for (int i = 0; i < peerManager->peerCount; i++) {
        if (peerManager->peers[i].workerId == targetWorkerId) {
            SOCKET peerSocket = peerManager->peers[i].socket;
            int isConnected = peerManager->peers[i].isConnected;
            LeaveCriticalSection(&peerManager->peerLock);
            
            if (isConnected && peerSocket != INVALID_SOCKET) {
                return peerSocket;
            } else {
                PrintWarning("Worker %d found in registry but not connected", targetWorkerId);
                return INVALID_SOCKET;
            }
        }
    }

    LeaveCriticalSection(&peerManager->peerLock);
    PrintError("Target worker %d not found in peer registry", targetWorkerId);
    return INVALID_SOCKET;
}


