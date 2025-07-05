#include "LoadBalancer.h"

int main(void) {

    int iResult;

    HANDLE inputHandlerThread = NULL;
    HANDLE workerListenerThread = NULL;
    HANDLE clientListenerThread = NULL;
    HANDLE workerClientRequestDispatcherThread = NULL;
    HANDLE clientWorkerResponseDispatcherThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL, NULL, NULL, NULL, NULL };

    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    Context context;
    memset(&context, 0, sizeof(Context));
    iResult = ContextInitialize(&context);
    if (iResult != 0) {
        PrintCritical("Failed to initialize the context.");

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    inputHandlerThread = CreateThread(NULL, 0, InputHandlerThread, &context, 0, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[0] = inputHandlerThread;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char workerPortString[6]; 
    snprintf(workerPortString, sizeof(workerPortString), "%d", SERVER_WORKER_PORT);
    iResult = getaddrinfo(NULL, workerPortString, &hints, &context.workerConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    char clientPortString[6]; 
    snprintf(clientPortString, sizeof(clientPortString), "%d", SERVER_CLIENT_PORT);
    iResult = getaddrinfo(NULL, clientPortString, &hints, &context.clientConnectionResultingAddress);
    if (iResult != 0) {
        PrintCritical("'getaddrinfo' failed with error: %d.", iResult);

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    context.workerListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context.workerListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    context.clientListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context.clientListenSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    iResult = bind(context.workerListenSocket, context.workerConnectionResultingAddress->ai_addr, (int)context.workerConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    iResult = bind(context.clientListenSocket, context.clientConnectionResultingAddress->ai_addr, (int)context.clientConnectionResultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'bind' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    freeaddrinfo(context.workerConnectionResultingAddress);
    context.workerConnectionResultingAddress = NULL;

    freeaddrinfo(context.clientConnectionResultingAddress);
    context.clientConnectionResultingAddress = NULL;

    iResult = listen(context.workerListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    iResult = listen(context.clientListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        PrintCritical("'listen' failed with error: %d.", WSAGetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }

    SetSocketNonBlocking(context.workerListenSocket);
    SetSocketNonBlocking(context.clientListenSocket);

    workerListenerThread = CreateThread(NULL, 0, &WorkerListenerThread, &context, 0, NULL);
    if (workerListenerThread == NULL) {
        PrintCritical("'CreateThread' for WorkerListenerThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[1] = workerListenerThread;

    clientListenerThread = CreateThread(NULL, 0, &ClientListenerThread, &context, 0, NULL);
    if (clientListenerThread == NULL) {
        PrintCritical("'CreateThread' for ClientListenerThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[2] = clientListenerThread;

    workerClientRequestDispatcherThread = CreateThread(NULL, 0, &WorkerClientRequestDispatcherThread, &context, 0, NULL);
    if (workerClientRequestDispatcherThread == NULL) {
        PrintCritical("'CreateThread' for WorkerClientRequestDispatcherThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[3] = workerClientRequestDispatcherThread;

    clientWorkerResponseDispatcherThread = CreateThread(NULL, 0, &ClientWorkerResponseDispatcherThread, &context, 0, NULL);
    if (clientWorkerResponseDispatcherThread == NULL) {
        PrintCritical("'CreateThread' for ClientWorkerResponseDispatcherThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[4] = clientWorkerResponseDispatcherThread;

    PrintInfo("Server initialized, waiting for clients and workers.");

    while (true) {
        if (GetFinishFlag(&context)) {

            break;
        }
    };

    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    CleanupFull(&context, threads, THREAD_COUNT);

    PrintInfo("Press any key to exit.");

    int _ = _getch(); 

    return EXIT_SUCCESS;
}

static void CleanupFull(Context* context, HANDLE threads[], int threadCount) {
    if (context->workerListenSocket != INVALID_SOCKET) {
        SafeCloseSocket(&context->workerListenSocket);
    }

    if (context->clientListenSocket != INVALID_SOCKET) {
        SafeCloseSocket(&context->clientListenSocket);
    }

    if (context->workerConnectionResultingAddress != NULL) {
        freeaddrinfo(context->workerConnectionResultingAddress);
    }

    if (context->clientConnectionResultingAddress != NULL) {
        freeaddrinfo(context->clientConnectionResultingAddress);
    }

    for (int i = 0; i < threadCount; i++) {
        if (threads[i] != NULL) {
            CloseHandle(threads[i]);
            threads[i] = NULL;
        }
    }

    ContextDestroy(context);

    WSACleanup();
}


