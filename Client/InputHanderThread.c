#include "InputHandlerThread.h"

DWORD WINAPI InputHandlerThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    char ch;

    PrintInfo("Press 'q' to stop the client.");

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        if (_kbhit()) {
            ch = _getch();

            while (_kbhit()) {
                _getch();
            }

            if (ch == 'q' || ch == 'Q') {
                PrintInfo("Shutdown signal received.");

                SetFinishSignal(context);

                break;
            }
        }

        Sleep(INPUT_WAIT_TIME);
    }

    return TRUE;
}


