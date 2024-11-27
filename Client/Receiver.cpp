#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Variable used to store function return value
    int iResult;

    // Socket used for communicating with the server
    SOCKET sock = *(SOCKET*)lpParam;

    PrintDebug("Receiver stopped.");

    return EXIT_SUCCESS;
}