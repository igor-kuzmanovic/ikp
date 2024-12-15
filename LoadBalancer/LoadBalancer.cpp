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
    PrintDebug("Resolving the server address and port for worker connections.");
    iResult = getaddrinfo(NULL, SERVER_WORKER_PORT, &hints, &ctx.workerConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Resolve the server address and port
    PrintDebug("Resolving the server address and port for client connections.");
    iResult = getaddrinfo(NULL, SERVER_CLIENT_PORT, &hints, &ctx.clientConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for worker connections
    PrintDebug("Creating the listen socket for worker connections.");
    ctx.workerListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.workerListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for client connections
    PrintDebug("Creating the listen socket for client connections.");
    ctx.clientListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.clientListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Bind the listen socket for worker connections
    PrintDebug("Binding the listen socket for worker connections.");
    iResult = bind(ctx.workerListenSocket, ctx.workerConnectionResultingAddress->ai_addr, (int)ctx.workerConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Bind the listen socket for client connections
    PrintDebug("Binding the listen socket for client connections.");
    iResult = bind(ctx.clientListenSocket, ctx.clientConnectionResultingAddress->ai_addr, (int)ctx.clientConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Since we don't need resultingAddress for worker connections anymore, free it
    freeaddrinfo(ctx.workerConnectionResultingAddress);
    ctx.workerConnectionResultingAddress = NULL;

    // Since we don't need resultingAddress for client connections anymore, free it
    freeaddrinfo(ctx.clientConnectionResultingAddress);
    ctx.clientConnectionResultingAddress = NULL;

    // Set worker listen socket to listening mode
    PrintDebug("Setting the listen socket for worker connections in listening mode.");
    iResult = listen(ctx.workerListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set client listen socket to listening mode
    PrintDebug("Setting the listen socket for client connections in listening mode.");
    iResult = listen(ctx.clientListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket for worker connections to non-blocking mode
    u_long socketWorkerMode = 1;
    PrintDebug("Setting the listen socket for worker connections to non-blocking mode.");
    iResult = ioctlsocket(ctx.workerListenSocket, FIONBIO, &socketWorkerMode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket for worker connections to non-blocking mode
    u_long socketClientMode = 1;
    PrintDebug("Setting the listen socket for client connections to non-blocking mode.");
    iResult = ioctlsocket(ctx.clientListenSocket, FIONBIO, &socketClientMode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&ctx, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    PrintInfo("Server initialized, waiting for clients and workers.");

    // An index to keep track of the accepted sockets
    int i = 0;

    while (true) {
        // Check stop signal
        if (GetFinishFlag(&ctx)) {
            PrintInfo("Stop signal received, stopping accepting new clients and workers.");

            break;
        }

        // Accept a worker socket
        SOCKET workerSocket = accept(ctx.workerListenSocket, NULL, NULL);
        if (workerSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }
        } else {
            PrintInfo("New worker connected.");

            // Create a structure to pass to the worker thread
            PrintDebug("Creating a new worker handler thread data structure.");
            WorkerHandlerThreadData* threadData = (WorkerHandlerThreadData*)malloc(sizeof(WorkerHandlerThreadData));
            if (!threadData) {
                PrintCritical("Memory allocation failed for worker thread data.");

                // Close everything and cleanup
                CleanupFull(&ctx, threads, THREAD_COUNT);

                return EXIT_FAILURE;
            }
            threadData->ctx = &ctx; // Pass the context pointer
            threadData->workerSocket = workerSocket; // Initialize the worker socket
            if (ctx.workerCount < MAX_WORKERS) {
                PrintDebug("Creating a new worker handler thread.");
                ctx.workerHandlerThreads[ctx.workerCount] = CreateThread(NULL, 0, WorkerHandlerThread, threadData, 0, NULL);
                if (ctx.workerHandlerThreads[ctx.workerCount] == NULL) {
                    PrintError("'CreateThread' failed with error: %d.", GetLastError());

                    // Close the worker socket
                    closesocket(workerSocket);

                    // Free the thread data memory
                    free(threadData);
                } else {
                    ctx.workerCount++;
                }
            } else {
                PrintWarning("Maximum worker limit reached. Rejecting worker.");

                // Close the worker socket
                closesocket(workerSocket);
            }
        }

        // Accept a client socket
        SOCKET clientSocket = accept(ctx.clientListenSocket, NULL, NULL);
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
                ctx.clientHandlerThreads[ctx.clientCount] = CreateThread(NULL, 0, ClientHandlerThread, threadData, 0, NULL);
                if (ctx.clientHandlerThreads[ctx.clientCount] == NULL) {
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

        // Cleanup finished worker handler threads
        for (int i = 0; i < ctx.workerCount; i++) {
            DWORD exitCode;
            if (GetExitCodeThread(ctx.workerHandlerThreads[i], &exitCode) && exitCode != STILL_ACTIVE) {
                PrintDebug("Worker handler thread %d has finished.", i);
                CloseHandle(ctx.workerHandlerThreads[i]);

                // Shift the last thread into the current slot
                ctx.workerHandlerThreads[i] = ctx.workerHandlerThreads[ctx.workerCount - 1];
                ctx.workerHandlerThreads[ctx.workerCount - 1] = NULL;
                ctx.workerCount--;
                i--; // Recheck the current index
            }
        }

        // Cleanup finished client handler threads
        for (int i = 0; i < ctx.clientCount; i++) {
            DWORD exitCode;
            if (GetExitCodeThread(ctx.clientHandlerThreads[i], &exitCode) && exitCode != STILL_ACTIVE) {
                PrintDebug("Client handler thread %d has finished.", i);
                CloseHandle(ctx.clientHandlerThreads[i]);

                // Shift the last thread into the current slot
                ctx.clientHandlerThreads[i] = ctx.clientHandlerThreads[ctx.clientCount - 1];
                ctx.clientHandlerThreads[ctx.clientCount - 1] = NULL;
                ctx.clientCount--;
                i--; // Recheck the current index
            }
        }

        Sleep(10); // Avoid busy waiting
    };

    // Wait for the client handler threads to finish
    for (int i = 0; i < ctx.clientCount; i++) {
        WaitForSingleObject(ctx.clientHandlerThreads[i], INFINITE);
    }

    // Wait for the worker handler threads to finish
    for (int i = 0; i < ctx.workerCount; i++) {
        WaitForSingleObject(ctx.workerHandlerThreads[i], INFINITE);
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
    // Close the listen socket for worker connections
    if (ctx->workerListenSocket != INVALID_SOCKET) {
        // Close the listen socket for worker connections
        PrintDebug("Closing the listen socket for worker connections.");
        if (closesocket(ctx->workerListenSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Close the listen socket for client connections
    if (ctx->clientListenSocket != INVALID_SOCKET) {
        // Close the listen socket for client connections
        PrintDebug("Closing the listen socket for client connections.");
        if (closesocket(ctx->clientListenSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Free address information for worker connections
    if (ctx->workerConnectionResultingAddress != NULL) {
        PrintDebug("Freeing address information.");
        freeaddrinfo(ctx->workerConnectionResultingAddress);
    }

    // Free address information for client connections
    if (ctx->clientConnectionResultingAddress != NULL) {
        PrintDebug("Freeing address information.");
        freeaddrinfo(ctx->clientConnectionResultingAddress);
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
        CloseHandle(ctx->clientHandlerThreads[i]);
        ctx->clientHandlerThreads[i] = NULL;
    }

    // Close the worker handler thread handles
    for (int i = 0; i < ctx->workerCount; i++) {
        PrintDebug("Closing worker handler thread handle %d.", i);
        CloseHandle(ctx->workerHandlerThreads[i]);
        ctx->workerHandlerThreads[i] = NULL;
    }

    // Cleanup the context
    PrintDebug("Cleaning up the context.");
    ContextCleanup(ctx);

    // Cleanup Winsock
    PrintDebug("Cleaning up Winsock.");
    WSACleanup();
}