#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    int iResult;

    // Socket used for communicating with the server
    SOCKET sock = *(SOCKET*)lpParam;

    PrintDebug("Receiver stopped.");

    return EXIT_SUCCESS;
}