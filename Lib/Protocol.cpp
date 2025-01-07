#include "Protocol.h"

int SerializeKVPair(const KeyValuePair* kvp, char* buffer) {
    if (!kvp || !buffer) {
        return -1;
    }

    // Format "key:value\n"
    int written = snprintf(buffer, BUFFER_SIZE, "%s:%s", kvp->key, kvp->value);
    if (written < 0 || (int)written >= BUFFER_SIZE) {
        // Error or buffer overflow
        return -1; 
    }

    return 0;
}

int DeserializeKVPair(const char* buffer, KeyValuePair* kvp) {
    if (!buffer || !kvp) {
        return -1;
    }

    const char* delimiter = strchr(buffer, ':');
    if (delimiter == NULL) {
        return -1; // Invalid format, no delimiter found
    }

    int keyLength = delimiter - buffer;
    if (keyLength >= MAX_KEY_LENGTH) {
        return -1; // Key too long
    }

    if (strncpy_s(kvp->key, MAX_KEY_LENGTH, buffer, keyLength) != 0) {
        return -1; // Error during copying
    }
    kvp->key[keyLength] = '\0';  // Ensure null-termination of the key

    const char* valueStart = delimiter + 1;

    int valueLength = strlen(valueStart);
    if (valueLength >= MAX_VALUE_LENGTH) {
        return -1; // Value too long
    }

    if (strncpy_s(kvp->value, MAX_VALUE_LENGTH, valueStart, valueLength) != 0) {
        return -1; // Error during copying
    }
    kvp->value[valueLength] = '\0';  // Ensure null-termination of the value

    if (kvp->key[0] == '\0' || kvp->value[0] == '\0') {
        return -1; // Invalid key-value pair
    }

    return 0;
}