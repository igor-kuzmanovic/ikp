#include "Client.h"

int main() {
    PrintDebug("Client started.");

    int iResult;

    // Threads to be created
    HANDLE inputHandlerThread = NULL;
    HANDLE senderThread = NULL;
    HANDLE receiverThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL, NULL, NULL };

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    PrintDebug("Initializing Winsock.");
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Initialize a context
    Context ctx{};
    PrintDebug("Initializing a context.");
    iResult = ContextInitialize(&ctx);
    if (iResult != 0) {
        PrintCritical("Failed to initialize the context.");

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Starts handling user input in a new thread
    PrintDebug("Starting input handler thread.");
    inputHandlerThread = CreateThread(NULL, 0, &InputHandlerThread, &ctx, NULL, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' for InputHandlerThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[0] = inputHandlerThread;

    // Create a socket for the client to connect to the server
    PrintDebug("Creating the connect socket.");
    ctx.connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.connectSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket to non-blocking mode
    u_long mode = 1;
    PrintDebug("Setting the connect socket to non-blocking mode.");
    iResult = ioctlsocket(ctx.connectSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create and initialize address structure
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddress.sin_port = htons(SERVER_PORT);

    // Connect to server specified in serverAddress and socket connectSocket
    PrintDebug("Connecting to the server.");
    iResult = connect(ctx.connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            PrintCritical("'connect' failed with error: %d.", WSAGetLastError());

            // Close everything and cleanup
            CleanupFull(&ctx, threads, THREAD_COUNT);

            return EXIT_FAILURE;
        }

        PrintDebug("Connection in progress.");
    }

    // Connect to server specified in serverAddress and socket connectSocket
    int retryCount = 0;
    while (retryCount < SERVER_CONNECT_MAX_RETRIES) {
        // Create a set of sockets to check for writability
        fd_set writeSet{};
        FD_ZERO(&writeSet);
        FD_SET(ctx.connectSocket, &writeSet);
        timeval timeout = { SERVER_CONNECT_TIMEOUT, 0 };  // SERVER_CONNECT_TIMEOUT seconds timeout

        // Use select() to wait for the socket to become writable (i.e., connection established)
        PrintDebug("Waiting for the connection to be established with 5 second timeout.");
        int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
        if (selectResult > 0 && FD_ISSET(ctx.connectSocket, &writeSet)) {
            PrintInfo("Client connected successfully.");

            break;
        }

        if (GetFinishFlag(&ctx)) {
            PrintInfo("Client stopped before connection was established.");

            // Close everything and cleanup
            CleanupFull(&ctx, threads, THREAD_COUNT);

            return EXIT_SUCCESS;
        } else {
            PrintError("Failed to connect after %d retries (timeout: %d seconds, retry interval: %d seconds).", retryCount, SERVER_CONNECT_TIMEOUT, SERVER_CONNECT_RETRY_INTERVAL);

            // Wait for SERVER_CONNECT_RETRY_INTERVAL seconds before retrying
            Sleep(SERVER_CONNECT_RETRY_INTERVAL);

            // Increment the retry count
            retryCount++;

            if (retryCount >= SERVER_CONNECT_MAX_RETRIES) {
                PrintCritical("Max connection retries exceeded.");

                // Close everything and cleanup
                CleanupFull(&ctx, threads, THREAD_COUNT);

                return EXIT_FAILURE;
            }
        }
    }

    // Starts periodically sending requests to the server in a new thread
    PrintDebug("Starting sender thread.");
    senderThread = CreateThread(NULL, 0, &SenderThread, &ctx, NULL, NULL);
    if (senderThread == NULL) {
        PrintCritical("'CreateThread' for SenderThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[1] = senderThread;

    // Starts receiving responses from the server in a new thread
    PrintDebug("Starting receiver thread.");
    receiverThread = CreateThread(NULL, 0, &ReceiverThread, &ctx, NULL, NULL);
    if (receiverThread == NULL) {
        PrintCritical("'CreateThread' for ReceiverThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[2] = receiverThread;

    // Wait for threads to finish
    PrintDebug("Waiting for the threads to finish.");
    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    // Close everything and cleanup
    CleanupFull(&ctx, threads, THREAD_COUNT);

    PrintDebug("Client stopped.");

    PrintInfo("Press any key to exit.");

    int _ = _getch(); // Wait for key press

    return EXIT_SUCCESS;
}


static void CleanupFull(Context* ctx, HANDLE threads[], int threadCount) {
    // Close the connect socket
    if (ctx->connectSocket != INVALID_SOCKET) {
        // Send shutdown to the server
        PrintDebug("Shutting down the connection.");
        if (shutdown(ctx->connectSocket, SD_BOTH) == SOCKET_ERROR) {
            PrintError("'shutdown' failed with error: %d.", WSAGetLastError());
        }

        // Close the connect socket
        PrintDebug("Closing the connect socket.");
        if (closesocket(ctx->connectSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Close the thread handles
    for (int i = 0; i < threadCount; i++) {
        if (threads[i] != NULL) {
            PrintDebug("Closing thread handle %d.", i);
            CloseHandle(threads[i]);
            threads[i] = NULL;
        }
    }

    // Cleanup the context
    PrintDebug("Cleaning up the context.");
    ContextCleanup(ctx);

    // Cleanup Winsock
    PrintDebug("Cleaning up Winsock.");
    WSACleanup();
}
