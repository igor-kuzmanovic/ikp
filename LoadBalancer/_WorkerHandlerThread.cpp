#include "_WorkerHandlerThread.h"

DWORD WINAPI WorkerHandlerThread(LPVOID lpParam) {
    PrintDebug("Worker handler started.");

    WorkerHandlerThreadData* threadData = (WorkerHandlerThreadData*)lpParam;

    // Access the socket and context
    SOCKET workerSocket = threadData->workerSocket;
    Context* ctx = threadData->ctx;

    int iResult;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(ctx->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping worker handler.");

            break;
        }

        // Receive data from the worker
        recvResult = recv(workerSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received: '%s' with length %d.", receiveBuffer, recvResult);

            // Message to reply with
            const char* replyMessage = "Replying to worker";

            // Respond to the worker
            PrintDebug("Replying to the worker.");
            sendResult = send(workerSocket, replyMessage, (int)strlen(replyMessage) + 1, 0);
            if (sendResult > 0) {
                PrintInfo("Reply sent: '%s' with length %d.", replyMessage, sendResult);
            } else if (sendResult == 0) {
                PrintInfo("Worker disconnected.");

                break;
            } else {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    // Ignore WSAEWOULDBLOCK, it is not an actual error
                    PrintError("[WorkerHandlerThread] 'send' failed with error: %d.", WSAGetLastError());

                    break;
                }

                continue;
            }
        } else if (recvResult == 0) {
            PrintInfo("Worker disconnected.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }

            continue;
        }
    }

    // Send shutdown notification
    PrintDebug("Notifying worker of server shutdown.");
    sendResult = send(workerSocket, SERVER_SHUTDOWN_MESSAGE, (int)strlen(SERVER_SHUTDOWN_MESSAGE) + 1, 0);
    if (sendResult == SOCKET_ERROR) {
        PrintError("[WorkerHandlerThread] 'send' failed with error: %d.", WSAGetLastError());
    }

    // Close the worker socket
    PrintDebug("Closing worker socket.");
    iResult = closesocket(workerSocket);
    if (iResult == SOCKET_ERROR) {
        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup the thread data
        free(threadData);

        return FALSE;
    }

    // Cleanup the thread data
    free(threadData);

    PrintDebug("Worker handler stopped.");

    return TRUE;
}