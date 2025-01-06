#include "ClientDataReceiverThread.h"

DWORD WINAPI ClientDataReceiverThread(LPVOID lpParam) {
    PrintDebug("Client data receiver started.");

    ClientDataReceiverThreadData* threadData = (ClientDataReceiverThreadData*)lpParam;

    SOCKET clientSocket = threadData->clientSocket;
    Context* ctx = threadData->ctx;
    int threadIndex = threadData->threadIndex;

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
            PrintDebug("Stop signal received, stopping client data receiver.");

            break;
        }

        // Receive data from the client
        recvResult = recv(clientSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received: '%s' with length %d.", receiveBuffer, recvResult);

            iResult = PutClientRequestQueue(ctx->clientRequestQueue, clientSocket, receiveBuffer);
            if (iResult == 0) {
                PrintError("Client request queue is full, notifying the client that the server is busy.");

                PrintDebug("Notifying the client that the server is busy.");
                send(clientSocket, SERVER_BUSY_MESSAGE, (int)strlen(SERVER_BUSY_MESSAGE) + 1, 0);

                PrintDebug("Sleeping for %d ms before accepting new requests.", CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);
                Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);

                continue;
            } else if (iResult == -1) {
                PrintError("Failed to put the request in the client request queue.");

                PrintDebug("Notifying the client that the server is busy.");
                send(clientSocket, SERVER_BUSY_MESSAGE, (int)strlen(SERVER_BUSY_MESSAGE) + 1, 0);

                PrintDebug("Sleeping for %d ms before accepting new requests.", CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);
                Sleep(CLIENT_REQUEST_QUEUE_FULL_SLEEP_TIME);

                continue;
            }
        } else if (recvResult == 0) {
            PrintInfo("Client disconnected.");

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
    PrintDebug("Notifying client of server shutdown.");
    sendResult = send(clientSocket, SERVER_SHUTDOWN_MESSAGE, (int)strlen(SERVER_SHUTDOWN_MESSAGE) + 1, 0);
    if (sendResult == SOCKET_ERROR) {
        PrintError("[ClientDataReceiverThread] 'send' failed with error: %d.", WSAGetLastError());
    }

    // Close the client socket
    PrintDebug("Closing client socket.");
    iResult = closesocket(clientSocket);
    if (iResult == SOCKET_ERROR) {
        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());
    }

    // Return the thread to the pool
    ReturnClientDataReceiverThread(ctx->clientThreadPool, threadIndex, threadData);

    PrintDebug("Client data receiver stopped.");

    return TRUE;
}