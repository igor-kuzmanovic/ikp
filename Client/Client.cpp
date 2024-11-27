#include "Client.h"

int main() {
    PrintDebug("Client started.");

    int iResult;

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintError("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Socket used for communicating with server
    SOCKET sock = INVALID_SOCKET;

    PrintInfo("Client connect socket is ready.");

    // An array to hold the sender and receiver threads
    HANDLE threads[2]{};

    // Starts periodically sending requests to the server in a new thread
    threads[0] = CreateThread(NULL, 0, &StartSender, &sock, NULL, NULL);
    if (threads[0] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close the socket
        // close socket...

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Starts receiving responses from the server in a new thread
    threads[1] = CreateThread(NULL, 0, &StartReceiver, &sock, NULL, NULL);
    if (threads[1] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close sender handle
        CloseHandle(threads[0]);

        // Close the socket
        // close socket...

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Shuts down the program when all the messages have been processed
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    // Send shutdown to the server
    // shutdown socket...

    // Close the sock
    // close socket...

    // Close the thread handles
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    // Cleanup Winsock
    WSACleanup();

    PrintDebug("Client stopped.");

    PrintInfo("Press any key to exit.");

    int _ = getchar(); // Wait for key press

    return EXIT_SUCCESS;
}
