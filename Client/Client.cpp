#include "Client.h"

int main() {
    // Variable used to store function return value
    int iResult;

    // Socket used for communicating with server
    SOCKET sock;

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP connecting socket
    sock = CreateConnectSocket(SERVER_ADDRESS, SERVER_PORT);
    if (sock == INVALID_SOCKET) {
        CloseSocket(sock);
        WSACleanup();

        return 1;
    }

    PrintInfo("Client connect socket is ready.");

    // An array to hold the sender and receiver threads
    HANDLE threads[2]{};

    // Starts periodically sending requests to the server in a new thread
    threads[0] = CreateThread(NULL, 0, &StartSender, &sock, NULL, NULL);
    if (threads[0] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close the socket
        CloseSocket(sock);

        // Cleanup Winsock
        WSACleanup();

        return 1;
    }

    // Starts receiving responses from the server in a new thread
    threads[1] = CreateThread(NULL, 0, &StartReceiver, &sock, NULL, NULL);
    if (threads[1] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close sender handle
        CloseHandle(threads[0]);

        // Close the socket
        CloseSocket(sock);

        // Cleanup Winsock
        WSACleanup();

        return 1;
    }

    // Shuts down the program when all the messages have been processed
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    // Send shutdown to the server
    ShutdownConnectSocket(sock);

    // Close the sock
    CloseSocket(sock);

    // Close the thread handles
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    // Cleanup Winsock
    WSACleanup();

    int _ = getchar(); // Wait for key press

    return 0;
}
