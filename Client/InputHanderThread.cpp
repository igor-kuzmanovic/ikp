#include "InputHandlerThread.h"

DWORD WINAPI InputHandlerThread(LPVOID lpParam) {
    // Shared context
    ClientContext* ctx = (ClientContext*)lpParam;

    PrintInfo("Press 'q' to stop the Client.");

    while (true) {
        if (_kbhit()) { // Check if a key is pressed
            char ch = _getch();
            if (ch == 'q') {
                PrintInfo("Shutdown signal received. Stopping the Client.");

                // Acquire lock to safely update the stop flag
                EnterCriticalSection(&ctx->lock);
                ctx->stopClient = true;
                LeaveCriticalSection(&ctx->lock);
                break;
            }
        }

        Sleep(10); // Avoid busy waiting
    }

    return EXIT_SUCCESS;
}