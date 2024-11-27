#include "Sender.h"

DWORD WINAPI StartSender(LPVOID lpParam) {
    PrintDebug("Sender started.");

    int iResult;

    // Socket used for communicating with the server
    SOCKET sock = *(SOCKET*)lpParam;

    PrintDebug("Sender stopped.");

    return EXIT_SUCCESS;
}