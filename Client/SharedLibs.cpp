#include "SharedLibs.h"

static int GetLocalPort(const SOCKET clientSocket) {
    struct sockaddr_in localAddr {};
    int addrLen = sizeof(localAddr);

    int localPort = 0;
    if (getsockname(clientSocket, (struct sockaddr*)&localAddr, &addrLen) == 0) {
        localPort = ntohs(localAddr.sin_port);
    } else {
        PrintError("[GetLocalPort] 'getsockname' failed with error: %d.", WSAGetLastError());

        return -1;
    }

    return localPort;
}

static int GenerateKey(char* key, const int localPort, const DWORD processId, const int id) {
    snprintf(key, MAX_KEY_LENGTH, "%d|%lu|%d", localPort, processId, id);

    int length = (int) strlen(key);

    return length;
}

static int GenerateRandomValue(char* value) {
    int randomValue = rand();
    int length = (randomValue % (MAX_VALUE_LENGTH - 1)) + MIN_VALUE_LENGTH;

    for (int i = 0; i < length; i++) {
        value[i] = ('A' + rand() % 26);
    }

    value[length] = '\0';

    value[0] = 'X'; // TODO Remove, for debugging purposes testing correct null termination
    value[length - 1] = 'X'; // TODO Remove, for debugging purposes testing correct null termination

    return length;
}

int GenerateClientMessage(Message *message, const SOCKET clientSocket, const int id) {
    struct sockaddr_in localAddr {};
    int addrLen = sizeof(localAddr);

    int localPort = GetLocalPort(clientSocket);
    if (localPort < 0) {
        PrintError("[GenerateClientMessage] Failed to get the local port.");

        return -1;
    }

    DWORD processId = GetCurrentProcessId();

    char key[MAX_KEY_LENGTH];
    int keyLength = GenerateKey(key, localPort, processId, id);
    if (keyLength < MIN_KEY_LENGTH) {
        PrintError("[GenerateClientMessage] Failed to generate a key.");

        return -1;
    } else if (keyLength > MAX_KEY_LENGTH) {
        PrintError("[GenerateClientMessage] Key length is too long: %d.", keyLength);

        return -1;
    }

    char value[MAX_VALUE_LENGTH];
    int valueLength = GenerateRandomValue(value);
    if (valueLength < MIN_VALUE_LENGTH) {
        PrintError("[GenerateClientMessage] Failed to generate a value.");

        return -1;
    } else if (valueLength > MAX_VALUE_LENGTH) {
        PrintError("[GenerateClientMessage] Value length is too long: %d.", valueLength);

        return -1;
    }

    KeyValuePairMessage keyValuePair{};
    snprintf(keyValuePair.key, keyLength + 1, "%s", key); // + 1 for null terminator
    snprintf(keyValuePair.value, valueLength + 1, "%s", value); // + 1 for null terminator

    message->type = MSG_KEY_VALUE_PAIR;
    message->length = keyLength + valueLength + 1; // + 1 for delimiter
    message->payload.keyValuePair = keyValuePair;

    return sizeof(MessageBuffer);
}