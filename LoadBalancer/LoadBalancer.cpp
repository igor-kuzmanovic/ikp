#include "LoadBalancer.h"

int main(void) {
    PrintDebug("Load Balancer started.");

    int iResult;

    // Threads to be created
    HANDLE inputHandlerThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL };

    // Initialize Winsock
    PrintDebug("Initializing Winsock.");
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Initialize a shared context
    Context ctx{};
    PrintDebug("Initializing a shared context.");
    iResult = ContextInitialize(&ctx);
    if (iResult != 0) {
        PrintCritical("Failed to initialize the context.");

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Start input handler thread
    PrintDebug("Starting the input handler thread.");
    inputHandlerThread = CreateThread(NULL, 0, InputHandlerThread, &ctx, 0, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[0] = inputHandlerThread;

    // Prepare address information structures
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    PrintDebug("Resolving the server address and port.");
    iResult = getaddrinfo(NULL, SERVER_PORT, &hints, &ctx.resultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for client connections
    PrintDebug("Creating the listen socket.");
    ctx.listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.listenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    PrintDebug("Binding the listen socket.");
    iResult = bind(ctx.listenSocket, ctx.resultingAddress->ai_addr, (int)ctx.resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(ctx.resultingAddress);
    ctx.resultingAddress = NULL;

    // Set listenSocket in listening mode
    PrintDebug("Setting the listen socket in listening mode.");
    iResult = listen(ctx.listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket to non-blocking mode
    u_long mode = 1;
    PrintDebug("Setting the listen socket to non-blocking mode.");
    iResult = ioctlsocket(ctx.listenSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    PrintInfo("Server initialized, waiting for clients.");

    // An index to keep track of the accepted sockets
    int i = 0;

    while(true) {
        // Check stop signal
        if (GetFinishFlag(&ctx)) {
            PrintInfo("Stop signal received, stopping accepting new clients.");

            break;
        }

        // Accept a client socket
        SOCKET clientSocket = accept(ctx.listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) { 
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }
        } else {
            PrintInfo("New client connected.");

            // Create a structure to pass to the client thread
            PrintDebug("Creating a new client handler thread data structure.");
            ClientHandlerThreadData* threadData = (ClientHandlerThreadData*)malloc(sizeof(ClientHandlerThreadData));
            if (!threadData) {
                PrintCritical("Memory allocation failed for thread data.");

                // Close everything and cleanup
                CleanupFull(&ctx, threads, THREAD_COUNT);

                return EXIT_FAILURE;
            }
            threadData->ctx = &ctx; // Pass the context pointer
            threadData->clientSocket = clientSocket; // Initialize the client socket
            if (ctx.clientCount < MAX_CLIENTS) {
                PrintDebug("Creating a new client handler thread.");
                ctx.clientThreads[ctx.clientCount] = CreateThread(NULL, 0, ClientHandlerThread, threadData, 0, NULL);
                if (ctx.clientThreads[ctx.clientCount] == NULL) {
                    PrintError("'CreateThread' failed with error: %d.", GetLastError());

                    // Close the client socket
                    closesocket(clientSocket);

                    // Free the thread data memory
                    free(threadData);
                } else {
                    ctx.clientCount++;
                }
            } else {
                PrintWarning("Maximum client limit reached. Rejecting client.");

                // Close the client socket
                closesocket(clientSocket);
            }
        }

        // Cleanup finished threads
        for (int i = 0; i < ctx.clientCount; i++) {
            DWORD exitCode;
            if (GetExitCodeThread(ctx.clientThreads[i], &exitCode) && exitCode != STILL_ACTIVE) {
                PrintDebug("Client handler thread %d has finished.", i);
                CloseHandle(ctx.clientThreads[i]);

                // Shift the last thread into the current slot
                ctx.clientThreads[i] = ctx.clientThreads[ctx.clientCount - 1];
                ctx.clientThreads[ctx.clientCount - 1] = NULL;
                ctx.clientCount--;
                i--; // Recheck the current index
            }
        }

        Sleep(10); // Avoid busy waiting
    };

    // Wait for the client handler threads to finish
    for (int i = 0; i < ctx.clientCount; i++) {
        WaitForSingleObject(ctx.clientThreads[i], INFINITE);
    }

    // Close everything and cleanup
    CleanupFull(&ctx, threads, THREAD_COUNT);

    PrintDebug("Load Balancer stopped.");

    PrintInfo("Press any key to exit.");

    int _ = _getch(); // Wait for key press

    return EXIT_SUCCESS;
}

// Full cleanup helper function
static void CleanupFull(Context* ctx, HANDLE threads[], int threadCount) {
    // Close the listen socket
    if (ctx->listenSocket != INVALID_SOCKET) {
        // Close the listen socket
        PrintDebug("Closing the listen socket.");
        if (closesocket(ctx->listenSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Free address information
    if (ctx->resultingAddress != NULL) {
        PrintDebug("Freeing address information.");
        freeaddrinfo(ctx->resultingAddress);
    }

    // Close the thread handles
    for (int i = 0; i < threadCount; i++) {
        if (threads[i] != NULL) {
            PrintDebug("Closing thread handle %d.", i);
            CloseHandle(threads[i]);
            threads[i] = NULL;
        }
    }

    // Close the client handler thread handles
    for (int i = 0; i < ctx->clientCount; i++) {
        PrintDebug("Closing client handler thread handle %d.", i);
        CloseHandle(ctx->clientThreads[i]);
        ctx->clientThreads[i] = NULL;
    }

    // Cleanup the context
    PrintDebug("Cleaning up the context.");
    ContextCleanup(ctx);

    // Cleanup Winsock
    PrintDebug("Cleaning up Winsock.");
    WSACleanup();
}