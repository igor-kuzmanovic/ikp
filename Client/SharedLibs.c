#include "SharedLibs.h"

void PrintVerificationSummary(Context* context) {
    EnterCriticalSection(&context->testData.lock);

    PrintInfo("===== VERIFICATION SUMMARY =====");
    PrintInfo("Total messages: %d", context->messageCount);
    PrintInfo("PUT success: %d/%d (%.1f%%)", context->testData.putSuccessCount, context->messageCount, (float)context->testData.putSuccessCount / context->messageCount * 100.0f);
    PrintInfo("GET success: %d/%d (%.1f%%)", context->testData.getSuccessCount, context->messageCount, (float)context->testData.getSuccessCount / context->messageCount * 100.0f);
    PrintInfo("================================");

    LeaveCriticalSection(&context->testData.lock);
}

int GetLocalPort(const SOCKET clientSocket) {
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(struct sockaddr_in));
    int addrLen = sizeof(localAddr);

    int localPort = 0;
    if (getsockname(clientSocket, (struct sockaddr*)&localAddr, &addrLen) == 0) {
        localPort = ntohs(localAddr.sin_port);
    } else {
        PrintError("'getsockname' failed with error: %d.", WSAGetLastError());
        return -1;
    }

    return localPort;
}

int GenerateKey(char* key, const int localPort, const DWORD processId, const int id) {
    snprintf(key, MAX_KEY_SIZE, "%d|%lu|%d", localPort, processId, id);
    return (int)strlen(key);
}

int GenerateRandomValue(char* value) {
    int randomValue = rand();
    int length = (randomValue % (MAX_VALUE_SIZE - 1)) + 1;

    for (int i = 0; i < length; i++) {
        value[i] = ('A' + rand() % 26);
    }
    value[length] = '\0';

    return length;
}


int GenerateAndSendClientMessage(SOCKET clientSocket, const int id) {
    int localPort = GetLocalPort(clientSocket);
    if (localPort < 0) {
        PrintError("Failed to get the local port.");
        return -1;
    }

    DWORD processId = GetCurrentProcessId();

    char key[MAX_KEY_SIZE + 1];
    int keyLength = GenerateKey(key, localPort, processId, id);
    if (keyLength <= 0 || keyLength > MAX_KEY_SIZE) {
        PrintError("Failed to generate a valid key, length: %d.", keyLength);
        return -1;
    }

    char value[MAX_VALUE_SIZE + 1];
    int valueLength = GenerateRandomValue(value);
    if (valueLength <= 0 || valueLength > MAX_VALUE_SIZE) {
        PrintError("Failed to generate a valid value, length: %d.", valueLength);
        return -1;
    }

    if (ValidateKey(key) == 0 || ValidateValue(value) == 0) {
        PrintError("Generated key or value is invalid.");
        return -1;
    }

    int result = SendPut(clientSocket, key, value);
    if (result > 0) {
        PrintInfo("PUT message sent: '%s':'%s' (%d bytes)", key, value, result);
    } else {
        PrintError("Failed to send PUT message for key '%s'", key);
    }

    return result;
}
