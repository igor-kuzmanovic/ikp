#include "Worker.h"
#include "ExportThread.h"

int main() {

    int iResult;

    int workerId = GetCurrentProcessId();
    int peerPort = WORKER_PEER_PORT_BASE + (workerId % 1000);
    
    PrintInfo("Worker Process ID: %d, Peer Port: %d", workerId, peerPort);

    HANDLE inputHandlerThread = NULL;
    HANDLE receiverThread = NULL;
    HANDLE peerListenerThread = NULL;
    HANDLE exportThread = NULL;
    HANDLE threads[THREAD_COUNT] = { NULL, NULL, NULL, NULL };

    iResult = InitializeWindowsSockets();
    if (iResult != 0) {
        PrintCritical("'WSAStartup' failed with error %d.", WSAGetLastError());

        return EXIT_FAILURE;
    }

    Context context;
    memset(&context, 0, sizeof(Context));
    iResult = ContextInitialize(&context, workerId, peerPort);
    if (iResult != 0) {
        PrintCritical("Failed to initialize the context.");

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
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

    struct    sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serverAddress.sin_port = htons(SERVER_WORKER_PORT);

    SetSocketNonBlocking(context.connectSocket);
    
    PrintInfo("Connecting to the server at %s:%d...", SERVER_ADDRESS, SERVER_WORKER_PORT);
    
    iResult = SafeConnect(context.connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress), 10);
    if (iResult != 0) {
        PrintInfo("Failed to connect to LoadBalancer. Make sure it is running.");
        PrintInfo("Press any key to exit.");
        int _ = _getch();
        CleanupFull(&context, threads, THREAD_COUNT);
        return EXIT_FAILURE;
    }

    PrintInfo("Worker connected successfully to LoadBalancer.");

    receiverThread = CreateThread(NULL, 0, &ReceiverThread, &context, 0, NULL);
    if (receiverThread == NULL) {
        PrintCritical("'CreateThread' for ReceiverThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[1] = receiverThread;

    peerListenerThread = CreateThread(NULL, 0, PeerListenerThread, &context, 0, NULL);
    if (peerListenerThread == NULL) {
        PrintCritical("'CreateThread' for PeerListenerThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[2] = peerListenerThread;
    
    exportThread = CreateThread(NULL, 0, ExportThread, &context, 0, NULL);
    if (exportThread == NULL) {
        PrintCritical("'CreateThread' for ExportThread failed with error: %d.", GetLastError());

        CleanupFull(&context, threads, THREAD_COUNT);

        return EXIT_FAILURE;
    }
    threads[3] = exportThread;
    
    Sleep(WORKER_STARTUP_DELAY);
    
    EnterCriticalSection(&context.lock);
    iResult = SendWorkerReady(context.connectSocket, workerId, peerPort);
    LeaveCriticalSection(&context.lock);
    
    if (iResult > 0) {
        PrintDebug("Worker ready message sent successfully (%d bytes).", iResult);
    } else {
        PrintWarning("Failed to send worker ready message, error: %d.", iResult);
    }

    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);

    CleanupFull(&context, threads, THREAD_COUNT);

    PrintInfo("Press any key to exit.");

    int _ = _getch(); 

    return EXIT_SUCCESS;
}

static void CleanupFull(Context* context, HANDLE threads[], int threadCount) {
    if (context->connectSocket != INVALID_SOCKET) {
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


