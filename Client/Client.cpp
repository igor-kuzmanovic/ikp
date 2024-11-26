#include "Client.h"

int main() {
    // Variable used to store function return value
    int iResult;

    // Socket used for communicating with server
    Connection connection;
    InitializeConnection(&connection);

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        return 1;
    }

    // Setup a TCP connecting socket
    iResult = CreateClientSocket(&connection, SERVER_ADDRESS, SERVER_PORT);
    if (iResult != 0) {
        CloseConnection(&connection);
        WSACleanup();

        return 1;
    }

    PrintInfo("Client connect socket is ready.");

    // An array to hold the sender and receiver threads
    HANDLE threads[2]{};

    // Starts periodically sending requests to the server in a new thread
    threads[0] = CreateThread(NULL, 0, &StartSender, &connection, NULL, NULL);
    if (threads[0] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close the connection
        CloseConnection(&connection);

        // Cleanup Winsock
        WSACleanup();

        return 1;
    }

    // Starts receiving responses from the server in a new thread
    threads[1] = CreateThread(NULL, 0, &StartReceiver, &connection, NULL, NULL);
    if (threads[1] == NULL) {
        PrintError("CreateThread failed with error: %d.", GetLastError());

        // Close sender handle
        CloseHandle(threads[0]);

        // Close the connection
        CloseConnection(&connection);

        // Cleanup Winsock
        WSACleanup();

        return 1;
    }

    // Shuts down the program when all the messages have been processed
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    // Send shutdown to the server
    ShutdownClient(&connection);

    // Close the connection to the server
    CloseConnection(&connection);

    // Close the thread handles
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    // Cleanup Winsock
    WSACleanup();

    int _ = getchar(); // Wait for key press

    return 0;
}
