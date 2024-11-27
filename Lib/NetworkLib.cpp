#include "NetworkLib.h"

int InitializeWindowsSockets() {
    int iResult;

    // Initialize windows sockets library for this process
    WSADATA wsaData;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return iResult;
    }

    return 0;
}
