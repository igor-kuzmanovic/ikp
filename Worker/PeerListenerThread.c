#include "PeerListenerThread.h"

DWORD WINAPI PeerListenerThread(LPVOID param) {
    Context* context = (Context*)param;
    if (!context || !context->peerManager) {
        PrintError("PeerListenerThread: Invalid parameter");
        return 1;
    }

    PeerManager* peerManager = context->peerManager;

    MessageType messageType;
    uint16_t actualSize;
    char buffer[MAX_MESSAGE_SIZE];

    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    int socketReady;

    peerManager->peerListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peerManager->peerListenSocket == INVALID_SOCKET) {
        PrintError("Failed to create peer listen socket");
        return 1;
    }

    SetSocketNonBlocking(peerManager->peerListenSocket);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(peerManager->peerPort);

    if (bind(peerManager->peerListenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        PrintError("Failed to bind peer listen socket to port %d", peerManager->peerPort);
        SafeCloseSocket(&peerManager->peerListenSocket);
        return 1;
    }

    if (listen(peerManager->peerListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        PrintError("Failed to listen on peer socket");
        SafeCloseSocket(&peerManager->peerListenSocket);
        return 1;
    }

    PrintInfo("Peer listener started on port %d", peerManager->peerPort);

    while (1) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        socketReady = IsSocketReadyToRead(peerManager->peerListenSocket, 100);

        if (socketReady == 1) {
            SOCKET clientSocket = SafeAccept(peerManager->peerListenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
            if (clientSocket == INVALID_SOCKET) {
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

            PrintInfo("Accepted peer connection from %s", clientIP);

            while (1) {
                if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
                    break;
                }

                int socketReady = IsSocketReadyToRead(clientSocket, 100);

                if (socketReady == 1) {
                    int recvResult = ProtocolReceive(clientSocket, &messageType, buffer, MAX_MESSAGE_SIZE, &actualSize);
                    if (recvResult != 0) {
                        PrintDebug("Peer connection from %s closed or error: %d", clientIP, recvResult);
                        break;
                    }

                    PrintDebug("Received message from peer %s: %s", clientIP, GetMessageTypeName(messageType));

                    switch (messageType) {
                    case MSG_DATA_EXPORT_START: {
                        uint32_t totalEntries;

                        if (ReceiveDataExportStart(buffer, actualSize, &totalEntries) == 0) {
                            PrintInfo("Receiving data export: %u entries", totalEntries);
                        } else {
                            PrintError("Failed to parse data export start message");
                        }
                        break;
                    }

                    case MSG_DATA_ENTRY: {
                        if (ReceiveDataEntry(buffer, actualSize, key, value) == 0) {
                            PrintInfo("Received data entry: '%s':'%s'", key, value);

                            if (!HasHashTable(context->hashTable, key)) {
                                if (SetHashTable(context->hashTable, key, value) == 1) {
                                    PrintDebug("Successfully stored data entry: %s:%s", key, value);
                                } else {
                                    PrintError("Failed to store data entry: %s:%s", key, value);
                                }
                            } else {
                                PrintDebug("Key '%s' already exists, ignoring data entry", key);
                            }
                        } else {
                            PrintError("Failed to parse data entry message");
                        }
                        break;
                    }

                    case MSG_PEER_NOTIFY: {
                        if (ReceivePeerNotify(buffer, actualSize, key, value) == 0) {
                            PrintInfo("Received peer data: '%s':'%s'", key, value);

                            if (!HasHashTable(context->hashTable, key)) {
                                if (SetHashTable(context->hashTable, key, value) == 1) {
                                    PrintDebug("Successfully stored peer data: %s:%s", key, value);
                                } else {
                                    PrintError("Failed to store peer data: %s:%s", key, value);
                                }
                            } else {
                                PrintDebug("Key '%s' already exists, ignoring peer data", key);
                            }
                        } else {
                            PrintError("Failed to parse peer notify message");
                        }
                        break;
                    }

                    case MSG_DATA_EXPORT_END: {
                        uint32_t memoryUsage;
                        int itemCount;
                        GetHashTableStats(context->hashTable, &memoryUsage, &itemCount);
                        PrintInfo("Received peer data export completed. Total items: %d, Memory: %u bytes (%u KB)",
                            itemCount, memoryUsage, memoryUsage / 1024);
                        break;
                    }

                    default:
                        PrintWarning("Unexpected message from peer %s: %s", clientIP, GetMessageTypeName(messageType));
                        break;
                    }
                }
            }

            SafeCloseSocket(&clientSocket);
        }

        Sleep(PEER_CONNECTION_RETRY_DELAY);
    }

    SafeCloseSocket(&peerManager->peerListenSocket);
    return 0;
}
