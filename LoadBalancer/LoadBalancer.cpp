#include "LoadBalancer.h"

int main(void) {
    PrintDebug("Load Balancer started.");

    int iResult;

    // Initialize Winsock
    PrintDebug("Initializing Winsock.");
    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Initialize a shared context
    LoadBalancerContext ctx{};
    PrintDebug("Initializing a shared context.");
    LoadBalancerContextInitialize(&ctx);

    // Socket used for communication with client
    SOCKET clientSocket = INVALID_SOCKET;

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    PrintDebug("Resolving the server address and port.");
    iResult = getaddrinfo(NULL, SERVER_PORT, &hints, &resultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Create a socket for the server to listen for client connections
    PrintDebug("Creating the listen socket.");
    ctx.listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.listenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Free address information
        freeaddrinfo(resultingAddress);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
    PrintDebug("Binding the listen socket.");
    iResult = bind(ctx.listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
        closesocket(ctx.listenSocket);

        // Free address information
        freeaddrinfo(resultingAddress);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    PrintDebug("Setting the listen socket in listening mode.");
    iResult = listen(ctx.listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
        closesocket(ctx.listenSocket);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Set listening socket to non-blocking mode
    u_long mode = 1;
    PrintDebug("Setting the listen socket to non-blocking mode.");
    iResult = ioctlsocket(ctx.listenSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
        closesocket(ctx.listenSocket);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Start input handler thread
    HANDLE inputHandlerThread = CreateThread(NULL, 0, InputHandlerThread, &ctx, 0, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close the listen socket
        closesocket(ctx.listenSocket);

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    PrintInfo("Server initialized, waiting for clients.");

    // An index to keep track of the accepted sockets
    int i = 0;

    while(true) {
        // Check stop signal
        if (ctx.stopServer) {
            PrintInfo("Stop signal received.");

            break;
        }

        // Accept a client socket
        clientSocket = accept(ctx.listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) { 
                // Ignore non-blocking "no connection" errors
                PrintError("'accept' failed with error: %d.", WSAGetLastError());
            }
        } else {
            PrintInfo("New client connected.");
            if (ctx.clientCount < MAX_CLIENTS) {
                // Create a structure to pass to the client thread
                PrintDebug("Creating a new client handler thread data structure.");
                ClientHandlerThreadData* threadData = (ClientHandlerThreadData*)malloc(sizeof(ClientHandlerThreadData));
                if (!threadData) {
                    PrintCritical("Memory allocation failed for thread data.");

                    // Close the listen socket
                    closesocket(ctx.listenSocket);

                    // Cleanup the context
                    LoadBalancerContextCleanup(&ctx);

                    // Cleanup Winsock
                    WSACleanup();

                    return EXIT_FAILURE;
                }
                threadData->clientSocket = clientSocket; // Pass the client socket
                threadData->ctx = &ctx; // Pass the context pointer

                PrintDebug("Creating a new client handler thread.");
                ctx.clientThreads[ctx.clientCount] = CreateThread(NULL, 0, ClientHandlerThread, threadData, 0, NULL);
                if (ctx.clientThreads[ctx.clientCount] == NULL) {
                    PrintError("'CreateThread' failed with error: %d.", GetLastError());

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
            GetExitCodeThread(ctx.clientThreads[i], &exitCode);
            if (exitCode != STILL_ACTIVE) {
                PrintDebug("Client handler thread %d has finished.", i);
                CloseHandle(ctx.clientThreads[i]);

                ctx.clientThreads[i] = ctx.clientThreads[ctx.clientCount - 1];
                ctx.clientCount--;
            }
        }

        Sleep(10); // Avoid busy waiting
    };

    // Wait for the client handler threads to finish
    for (int i = 0; i < ctx.clientCount; i++) {
        WaitForSingleObject(ctx.clientThreads[i], INFINITE);
        CloseHandle(ctx.clientThreads[i]);
    }

    // Close the listen socket
    PrintDebug("Closing the listen socket.");
    iResult = closesocket(ctx.listenSocket);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup the context
        LoadBalancerContextCleanup(&ctx);

        // Close the input handler thread
        CloseHandle(inputHandlerThread);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Cleanup the context
    LoadBalancerContextCleanup(&ctx);

    // Close the input handler thread
    CloseHandle(inputHandlerThread);

    // Cleanup Winsock
    WSACleanup();

    PrintDebug("Load Balancer stopped.");

    PrintInfo("Press any key to exit.");

    int _ = getchar(); // Wait for key press

    return EXIT_SUCCESS;
}

int LoadBalancerContextInitialize(LoadBalancerContext* ctx)
{
    ctx->stopServer = false;
    ctx->listenSocket = INVALID_SOCKET;
    ctx->clientCount = 0;
    InitializeCriticalSection(&ctx->lock);

    return 0;
}

int LoadBalancerContextCleanup(LoadBalancerContext* ctx)
{
    DeleteCriticalSection(&ctx->lock);

    return 0;
}