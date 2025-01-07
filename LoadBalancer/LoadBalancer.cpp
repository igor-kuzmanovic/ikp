#include "LoadBalancer.h"

int main(void) {
    PrintDebug("Load balancer started.");

    int iResult;

    // Threads to be created
    HANDLE inputHandlerThread = NULL;
    HANDLE workerListenerThread = NULL;
    HANDLE clientListenerThread = NULL;
    HANDLE workerClientRequestDispatcherThread = NULL;
    HANDLE workerHealthCheckThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL, NULL, NULL, NULL, NULL };

    // Initialize Winsock
    PrintDebug("Initializing Winsock.");
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Initialize a shared context
    Context context{};
    PrintDebug("Initializing a shared context.");
    iResult = ContextInitialize(&context);
    if (iResult != 0) {
        PrintCritical("Failed to initialize the context.");

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Start input handler thread
    PrintDebug("Starting the input handler thread.");
    inputHandlerThread = CreateThread(NULL, 0, InputHandlerThread, &context, 0, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

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
    char workerPortString[6]; // Need 6 characters to store the port number
    snprintf(workerPortString, sizeof(workerPortString), "%d", SERVER_WORKER_PORT);
    iResult = getaddrinfo(NULL, workerPortString, &hints, &context.workerConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Resolve the server address and port
    PrintDebug("Resolving the server address and port for client connections.");
    char clientPortString[6]; // Need 6 characters to store the port number
    snprintf(clientPortString, sizeof(clientPortString), "%d", SERVER_CLIENT_PORT);
    iResult = getaddrinfo(NULL, clientPortString, &hints, &context.clientConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for worker connections
    PrintDebug("Creating the listen socket for worker connections.");
    context.workerListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context.workerListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for client connections
    PrintDebug("Creating the listen socket for client connections.");
    context.clientListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context.clientListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Bind the listen socket for worker connections
    PrintDebug("Binding the listen socket for worker connections.");
    iResult = bind(context.workerListenSocket, context.workerConnectionResultingAddress->ai_addr, (int)context.workerConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Bind the listen socket for client connections
    PrintDebug("Binding the listen socket for client connections.");
    iResult = bind(context.clientListenSocket, context.clientConnectionResultingAddress->ai_addr, (int)context.clientConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Since we don't need resultingAddress for worker connections anymore, free it
    freeaddrinfo(context.workerConnectionResultingAddress);
    context.workerConnectionResultingAddress = NULL;

    // Since we don't need resultingAddress for client connections anymore, free it
    freeaddrinfo(context.clientConnectionResultingAddress);
    context.clientConnectionResultingAddress = NULL;

    // Set worker listen socket to listening mode
    PrintDebug("Setting the listen socket for worker connections in listening mode.");
    iResult = listen(context.workerListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set client listen socket to listening mode
    PrintDebug("Setting the listen socket for client connections in listening mode.");
    iResult = listen(context.clientListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket for worker connections to non-blocking mode
    u_long socketWorkerMode = 1;
    PrintDebug("Setting the listen socket for worker connections to non-blocking mode.");
    iResult = ioctlsocket(context.workerListenSocket, FIONBIO, &socketWorkerMode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Set listening socket for client connections to non-blocking mode
    u_long socketClientMode = 1;
    PrintDebug("Setting the listen socket for client connections to non-blocking mode.");
    iResult = ioctlsocket(context.clientListenSocket, FIONBIO, &socketClientMode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    // Starts listening for workers in a new thread
    PrintDebug("Starting worker listener thread.");
    workerListenerThread = CreateThread(NULL, 0, &WorkerListenerThread, &context, NULL, NULL);
    if (workerListenerThread == NULL) {
        PrintCritical("'CreateThread' for WorkerListenerThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[1] = workerListenerThread;

    // Starts listening for clients in a new thread
    PrintDebug("Starting client listener thread.");
    clientListenerThread = CreateThread(NULL, 0, &ClientListenerThread, &context, NULL, NULL);
    if (clientListenerThread == NULL) {
        PrintCritical("'CreateThread' for ClientListenerThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[2] = clientListenerThread;

    // Starts dispatching client requests to workers in a new thread
    PrintDebug("Starting worker client request dispatcher thread.");
    workerClientRequestDispatcherThread = CreateThread(NULL, 0, &WorkerClientRequestDispatcherThread, &context, NULL, NULL);
    if (workerClientRequestDispatcherThread == NULL) {
        PrintCritical("'CreateThread' for WorkerClientRequestDispatcherThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[3] = workerClientRequestDispatcherThread;

    // Starts the worker health check thread
    PrintDebug("Starting worker health check thread.");
    workerHealthCheckThread = CreateThread(NULL, 0, &WorkerHealthCheckThread, &context, NULL, NULL);
    if (workerHealthCheckThread == NULL) {
        PrintCritical("'CreateThread' for WorkerHealthCheckThread failed with error: %d.", GetLastError());

        // Close everything and cleanup
        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[4] = workerHealthCheckThread;

    PrintInfo("Server initialized, waiting for clients and workers.");

    while (true) {
        // Check stop signal
        if (GetFinishFlag(&context)) {
            PrintDebug("Stop signal received, stopping accepting new clients and workers.");

            break;
        }
    };

    // Wait for threads to finish
    PrintDebug("Waiting for the threads to finish.");
    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    // Close everything and cleanup
    CleanupFull(&context, threads, THREAD_COUNT);

    PrintDebug("Load balancer stopped.");

    PrintInfo("Press any key to exit.");

    int _ = _getch(); // Wait for key press

    return EXIT_SUCCESS;
}

// Full cleanup helper function
static void CleanupFull(Context* context, HANDLE threads[], int threadCount) {
    // Close the listen socket for worker connections
    if (context->workerListenSocket != INVALID_SOCKET) {
        // Close the listen socket for worker connections
        PrintDebug("Closing the listen socket for worker connections.");
        if (closesocket(context->workerListenSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Close the listen socket for client connections
    if (context->clientListenSocket != INVALID_SOCKET) {
        // Close the listen socket for client connections
        PrintDebug("Closing the listen socket for client connections.");
        if (closesocket(context->clientListenSocket) == SOCKET_ERROR) {
            PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
        }
    }

    // Free address information for worker connections
    if (context->workerConnectionResultingAddress != NULL) {
        PrintDebug("Freeing address information.");
        freeaddrinfo(context->workerConnectionResultingAddress);
    }

    // Free address information for client connections
    if (context->clientConnectionResultingAddress != NULL) {
        PrintDebug("Freeing address information.");
        freeaddrinfo(context->clientConnectionResultingAddress);
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
    ContextDestroy(context);

    // Cleanup Winsock
    PrintDebug("Cleaning up Winsock.");
    WSACleanup();
}