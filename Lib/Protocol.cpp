#include "Protocol.h"

int SerializeKVPair(const KeyValuePair* kvp, char* buffer, size_t buffer_size) {
    if (!kvp || !buffer) {
        return -1;
    }

    // Make sure strings are null-terminated
    if (kvp->key[MAX_KEY_LENGTH - 1] != '\0' || kvp->value[MAX_VALUE_LENGTH - 1] != '\0') {
        // Key or value not properly null-terminated
        return -1;
    }

    // Format "key:value\n"
    int written = snprintf(buffer, buffer_size, "%s:%s\n", kvp->key, kvp->value);
    if (written < 0 || (size_t)written >= buffer_size) {
        // Error or buffer overflow
        return -1; 
    }

    return 0;
}

int DeserializeKVPair(const char* buffer, KeyValuePair* kvp) {
    if (!buffer || !kvp) {
        return -1;
    }

    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];

    // Parse the buffer with ":" as the delimiter
    if (sscanf_s(buffer, "%255[^:]:%755[^\n]", key, (unsigned int)sizeof(key), value, (unsigned int)sizeof(value)) != 2) {
        // Parsing failed
        return -1;
    }

    // Copy key and value to the struct with proper null-termination
    snprintf(kvp->key, MAX_KEY_LENGTH, "%s", key);
    snprintf(kvp->value, MAX_VALUE_LENGTH, "%s", value);

    return 0;
}