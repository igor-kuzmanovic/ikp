#include "WorkerListenerThread.h"

DWORD WINAPI WorkerListenerThread(LPVOID lpParam) {

    Context* context = (Context*)lpParam;

    int iResult;
    int workerId = 1;
    char workerAddress[256];
    int socketReady;
    SOCKET workerSocket;

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        socketReady = IsSocketReadyToRead(context->workerListenSocket, 100);

        if (socketReady == 1) {
            workerSocket = SafeAccept(context->workerListenSocket, NULL, NULL);
            if (workerSocket == INVALID_SOCKET) {
                continue;
            }

            PrintInfo("New worker connected.");

            GetWorkerAddress(workerSocket, workerAddress, sizeof(workerAddress));
            PrintInfo("Worker from %s connected", workerAddress);

            if (context->workerThreadPool->count < MAX_WORKERS) {
                iResult = AssignWorkerDataReceiverThread(context->workerThreadPool, workerSocket, context, -1);
                if (iResult == -1) {
                    PrintWarning("Cannot assign worker to a worker data receiver thread. Rejecting worker.");

                    iResult = SafeCloseSocket(&workerSocket);
                    if (iResult != 0) {
                        PrintError("SafeCloseSocket failed with error: %d.", iResult);
                    }
                } else {
                    PrintInfo("Worker successfully added to worker list, waiting for ready message");
                }
            } else {
                PrintWarning("Maximum worker limit reached. Rejecting worker.");

                iResult = SafeCloseSocket(&workerSocket);
                if (iResult != 0) {
                    PrintError("SafeCloseSocket failed with error: %d.", iResult);
                }
            }
        }
    }

    return TRUE;
}


