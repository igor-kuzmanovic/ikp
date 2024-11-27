#include "Worker.h"

int main(void) {
    PrintDebug("Worker started.");

    int iResult;

    PrintDebug("Worker stopped.");

    PrintInfo("Press any key to exit.");

    int _ = getchar(); // Wait for key press

    return EXIT_SUCCESS;
}