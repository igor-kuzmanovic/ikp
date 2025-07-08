#include "Client.h"

int main(int argc, char* argv[]) {
    int iResult;

    HANDLE inputHandlerThread = NULL;
    HANDLE senderThread = NULL;
    HANDLE receiverThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL, NULL, NULL };

    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
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

    context.messageCount = CLIENT_MESSAGE_COUNT;
    if (argc > 1) {
        int count = atoi(argv[1]);
        if (count > 0) {
            context.messageCount = count;
        } else {
            PrintWarning("Invalid message count '%s'. Using default value (%d).", argv[1], CLIENT_MESSAGE_COUNT);
        }
    }

    inputHandlerThread = CreateThread(NULL, 0, &InputHandlerThread, &context, 0, NULL);
    if (inputHandlerThread == NULL) {
        PrintCritical("'CreateThread' for InputHandlerThread failed with error: %d.", GetLastError());
        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }
    threads[0] = inputHandlerThread;

    context.connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context.connectSocket == INVALID_SOCKET) {
        PrintCritical("'socket' failed with error: %d.", WSAGetLastError());
        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddress.sin_port = htons(SERVER_CLIENT_PORT);

    SetSocketNonBlocking(context.connectSocket);

    PrintInfo("Connecting to the server at %s:%d...", SERVER_ADDRESS, SERVER_CLIENT_PORT);

    iResult = SafeConnect(context.connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress), 10);
    if (iResult != 0) {
        PrintCritical("Failed to connect to LoadBalancer. Make sure it is running.");

        PrintInfo("Press any key to exit.");
        int _ = _getch();

        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }

    PrintInfo("Client connected successfully to LoadBalancer.");

    senderThread = CreateThread(NULL, 0, &SenderThread, &context, 0, NULL);
    if (senderThread == NULL) {
        PrintCritical("'CreateThread' for SenderThread failed with error: %d.", GetLastError());
        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }
    threads[1] = senderThread;

    receiverThread = CreateThread(NULL, 0, &ReceiverThread, &context, 0, NULL);
    if (receiverThread == NULL) {
        PrintCritical("'CreateThread' for ReceiverThread failed with error: %d.", GetLastError());
        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }
    threads[2] = receiverThread;

    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    PrintVerificationSummary(&context);

    CleanupFull(&context, threads, THREAD_COUNT);

    PrintInfo("Press any key to exit.");
    int _ = _getch();

    return EXIT_SUCCESS;
}

static void CleanupFull(Context* context, HANDLE threads[], int threadCount) {
    if (context->connectSocket != INVALID_SOCKET) {
        if (shutdown(context->connectSocket, SD_BOTH) == SOCKET_ERROR) {
            PrintError("'shutdown' failed with error: %d.", WSAGetLastError());
        }

        SafeCloseSocket(&context->connectSocket);
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


