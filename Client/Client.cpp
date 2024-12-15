#include "Client.h"

int main() {
    PrintDebug("Client started.");

    int iResult;

    // Initialize Winsock
    iResult = InitializeWindowsSockets();
    PrintDebug("Initializing Winsock.");
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    // Initialize a shared context
    ClientContext ctx{};
    PrintDebug("Initializing a shared context.");
    ClientContextInitialize(&ctx);

    // Create a socket for the client to connect to the server
    PrintDebug("Creating the connect socket.");
    ctx.connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx.connectSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Set listening socket to non-blocking mode
    u_long mode = 1;
    PrintDebug("Setting the connect socket to non-blocking mode.");
    iResult = ioctlsocket(ctx.connectSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

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

            // Close the connect socket
            closesocket(ctx.connectSocket);

            // Cleanup the context
            ClientContextCleanup(&ctx);

            // Cleanup Winsock
            WSACleanup();

            return EXIT_FAILURE;
        }

        PrintDebug("Connection in progress.");
    }

    // Create an event to monitor the socket state
    FD_SET writeSet{};
    FD_ZERO(&writeSet);
    FD_SET(ctx.connectSocket, &writeSet);

    // Use select() to wait for the socket to become writable (i.e., connection established)
    timeval timeout = { 5, 0 };  // 5 seconds timeout
    PrintDebug("Waiting for the connection to be established.");
    int selectResult = select(0, NULL, &writeSet, NULL, &timeout);
    if (selectResult > 0 && FD_ISSET(ctx.connectSocket, &writeSet)) {
        PrintInfo("Client connected successfully.");
    } else {
        PrintCritical("Failed to connect within timeout.");

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Starts handling user input in a new thread
    PrintDebug("Starting input handler thread.");
    HANDLE inputHandlerThread = CreateThread(NULL, 0, &InputHandlerThread, &ctx, NULL, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Starts periodically sending requests to the server in a new thread
    PrintDebug("Starting sender thread.");
    HANDLE senderThread = CreateThread(NULL, 0, &SenderThread, &ctx, NULL, NULL);
    if (senderThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close the input handler thread
        CloseHandle(inputHandlerThread);

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Starts receiving responses from the server in a new thread
    PrintDebug("Starting receiver thread.");
    HANDLE receiverThread = CreateThread(NULL, 0, &ReceiverThread, &ctx, NULL, NULL);
    if (receiverThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close the input handler thread
        CloseHandle(inputHandlerThread);
        
        // Close the sender thread handle
        CloseHandle(senderThread);

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Wait for threads to finish
    PrintDebug("Waiting for the threads to finish.");
    HANDLE threads[3] = { inputHandlerThread, senderThread, receiverThread };
    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    // Send shutdown to the server
    PrintDebug("Shutting down the connection.");
    iResult = shutdown(ctx.connectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'shutdown' failed with error: %d.", WSAGetLastError());

        // Close the input handler thread
        CloseHandle(inputHandlerThread);

        // Close the sender thread handle
        CloseHandle(senderThread);

        // Close the receiver thread handle
        CloseHandle(receiverThread);

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Close the input handler thread
    CloseHandle(inputHandlerThread);

    // Close the sender thread handle
    CloseHandle(senderThread);

    // Close the receiver thread handle
    CloseHandle(receiverThread);

    // Close the connect socket
    iResult = closesocket(ctx.connectSocket);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Cleanup the context
    ClientContextCleanup(&ctx);

    // Cleanup Winsock
    WSACleanup();

    PrintDebug("Client stopped.");

    PrintInfo("Press any key to exit.");

    int _ = _getch(); // Wait for key press

    return EXIT_SUCCESS;
}

int ClientContextInitialize(ClientContext* ctx) {
    ctx->stopClient = false;
    ctx->connectSocket = INVALID_SOCKET;
    InitializeCriticalSection(&ctx->lock);

    return 0;
}

int ClientContextCleanup(ClientContext* ctx) {
    DeleteCriticalSection(&ctx->lock);

    return 0;
}
