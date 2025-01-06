#include "Worker.h"

int main() {
    PrintDebug("Worker started.");

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

    // Create a socket for the worker to connect to the server
    PrintDebug("Creating the connect socket.");
    ctx.connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.connectSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the worker to connect to the server
    PrintDebug("Creating the connect socket.");
    ctx.connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.connectSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create and initialize address structure
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddress.sin_port = htons(SERVER_WORKER_PORT);

    // Retry logic for connecting to the server
    int retryCount = 1;
    while (retryCount < SERVER_CONNECT_MAX_RETRIES) {
        PrintDebug("Connecting to the server (Attempt %d/%d).", retryCount, SERVER_CONNECT_MAX_RETRIES);

        // Connect to server specified in serverAddress and socket connectSocket
        iResult = connect(ctx.connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
        if (iResult == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            if (errorCode == WSAECONNREFUSED) {
                // Handle connection refused, server might not be up yet
                PrintError("Connection refused by the server (error %d).", errorCode);
            } else if (errorCode == WSAETIMEDOUT) {
                // Handle connection timeout
                PrintError("Connection attempt timed out (error %d).", errorCode);
            } else {
                // Handle other errors
                PrintError("Failed to connect with error %d.", errorCode);
            }

            // Increment the retry count
            retryCount++;
            if (retryCount <= SERVER_CONNECT_MAX_RETRIES) {
                // Wait for a short period before retrying (retry interval)
                PrintDebug("Waiting for %d seconds before retrying...", SERVER_CONNECT_RETRY_INTERVAL);

                Sleep(SERVER_CONNECT_RETRY_INTERVAL);
            }
        } else {
            // Successfully connected
            PrintInfo("Worker connected successfully.");

            break; // Exit the loop on successful connection
        }

        // If retry limit reached, exit with failure
        if (retryCount > SERVER_CONNECT_MAX_RETRIES) {
            PrintCritical("Max connection retries exceeded.");

            // Close everything and cleanup
            CleanupFull(&ctx, threads, THREAD_COUNT);

            return EXIT_FAILURE;
        }
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
    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    // Close everything and cleanup
    CleanupFull(&ctx, threads, THREAD_COUNT);

    PrintDebug("Worker stopped.");

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
    ContextDestroy(ctx);

    // Cleanup Winsock
    PrintDebug("Cleaning up Winsock.");
    WSACleanup();
}
