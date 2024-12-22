#include "SharedLibs.h"

int GenerateClientMessage(SOCKET clientSocket, char* buffer, size_t bufferSize, int id) {
    struct sockaddr_in localAddr {};
    int addrLen = sizeof(localAddr);

    // Get the local port
    int localPort = 0;
    if (getsockname(clientSocket, (struct sockaddr*)&localAddr, &addrLen) == 0) {
        localPort = ntohs(localAddr.sin_port);
    } else {
        PrintError("'getsockname' failed with error: %d.", WSAGetLastError());

        return -1;
    }

    // Get the process ID
    DWORD processId = GetCurrentProcessId();

    // Format the message
    int written = snprintf(buffer, bufferSize, "Hello from Client! Port: %d, PID: %lu, ID: %ld", localPort, processId, id);

    // Check if there is a message truncation or formatting error
    if (written < 0 || (size_t)written >= bufferSize) {
        return -1;
    }

    return 0;
}