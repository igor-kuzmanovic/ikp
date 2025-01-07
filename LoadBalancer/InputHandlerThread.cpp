#include "InputHandlerThread.h"

DWORD WINAPI InputHandlerThread(LPVOID lpParam) {
    PrintDebug("Input handler started.");

    // Context
    Context* context = (Context*)lpParam;

    PrintInfo("Press 'q' to stop the load balancer.");

    while (true) {
        if (_kbhit()) { // Check if a key is pressed
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                PrintDebug("Shutdown signal received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(context);

                break;
            }
        }

        Sleep(INPUT_WAIT_TIME);
    }

    PrintDebug("Input handler stopped.");

    return TRUE;
}