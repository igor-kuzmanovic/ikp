#include "InputHandlerThread.h"

DWORD WINAPI InputHandlerThread(LPVOID lpParam) {
    // Context
    Context* ctx = (Context*)lpParam;

    PrintInfo("Press 'q' to stop the load balancer.");

    while (true) {
        if (_kbhit()) { // Check if a key is pressed
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                PrintInfo("Shutdown signal received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(ctx);

                break;
            }
        }

        Sleep(10); // Avoid busy waiting
    }

    return EXIT_SUCCESS;
}