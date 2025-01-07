#include "SharedLibs.h"

int GenerateClientMessage(SOCKET clientSocket, char* buffer, int id) {
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

    // Create the key as a combination of port, process ID, and message ID
    char key[MAX_KEY_LENGTH];
    snprintf(key, MAX_KEY_LENGTH, "%d|%lu|%d", localPort, processId, id);

    // Store the value of `rand`
    int randomValue = rand();

    // Generate a random value for the key-value pair
    // Ensure valueLength is always at least 1
    int valueLength = (randomValue % (MAX_VALUE_LENGTH - 1)) + 1; // Random length between 1 and MAX_VALUE_LENGTH - 1
    char value[MAX_VALUE_LENGTH]{};  // Initialize the value array

    // Fill the value with random characters
    for (int i = 0; i < valueLength; i++) {
        value[i] = ('A' + rand() % 26); // Random uppercase letters
    }

    // Null-terminate the value
    value[valueLength] = '\0';  // Null-terminate the string

    value[0] = 'X'; // TODO Remove, for debugging purposes testing correct null termination
    value[valueLength - 1] = 'X'; // TODO Remove, for debugging purposes testing correct null termination

    // Create the KeyValuePair
    KeyValuePair kvp{};
    snprintf(kvp.key, MAX_KEY_LENGTH, "%s", key);
    snprintf(kvp.value, MAX_VALUE_LENGTH, "%s", value);

    // Serialize the KeyValuePair into the buffer
    if (SerializeKVPair(&kvp, buffer) != 0) {
        PrintError("Serialization failed.");

        return -1;
    }

    return 0;
}