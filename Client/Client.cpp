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

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Set listening socket to non-blocking mode
    u_long mode = 1;
    PrintDebug("Setting the listen socket to non-blocking mode.");
    iResult = ioctlsocket(ctx.connectSocket, FIONBIO, &mode);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'ioctlsocket' failed with error: %d.", WSAGetLastError());

        // Close the listen socket
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
        PrintCritical("'connect' failed with error: %d.", WSAGetLastError());

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup Winsock
        WSACleanup();
    }

    PrintInfo("Client connect socket is ready.");

    // An array to hold the sender and receiver threads
    HANDLE threads[3]{};

    // Starts periodically sending requests to the server in a new thread
    PrintDebug("Starting sender thread.");
    threads[0] = CreateThread(NULL, 0, &SenderThread, &ctx, NULL, NULL);
    if (threads[0] == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Starts receiving responses from the server in a new thread
    PrintDebug("Starting receiver thread.");
    threads[1] = CreateThread(NULL, 0, &ReceiverThread, &ctx, NULL, NULL);
    if (threads[1] == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close sender handle
        CloseHandle(threads[0]);

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
    threads[2] = CreateThread(NULL, 0, &InputHandlerThread, &ctx, NULL, NULL);
    if (threads[2] == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        // Close sender and receiver handles
        CloseHandle(threads[0]);
        CloseHandle(threads[1]);

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Shuts down the program when all the messages have been processed
    PrintDebug("Waiting for the threads to finish.");
    WaitForMultipleObjects(3, threads, TRUE, INFINITE);

    // Send shutdown to the server
    PrintDebug("Shutting down the connection.");
    iResult = shutdown(ctx.connectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'shutdown' failed with error: %d.", WSAGetLastError());

        // Close the connect socket
        closesocket(ctx.connectSocket);

        // Close the thread handles
        CloseHandle(threads[0]);
        CloseHandle(threads[1]);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Close the connect socket
    iResult = closesocket(ctx.connectSocket);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'closesocket' failed with error: %d.", WSAGetLastError());

        // Close the thread handles
        CloseHandle(threads[0]);
        CloseHandle(threads[1]);

        // Cleanup the context
        ClientContextCleanup(&ctx);

        // Cleanup Winsock
        WSACleanup();

        return EXIT_FAILURE;
    }

    // Close the thread handles
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    CloseHandle(threads[2]);

    // Cleanup the context
    ClientContextCleanup(&ctx);

    // Cleanup Winsock
    WSACleanup();

    PrintDebug("Client stopped.");

    PrintInfo("Press any key to exit.");

    int _ = getchar(); // Wait for key press

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
