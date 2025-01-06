#include "_ClientHandlerThread.h"

DWORD WINAPI ClientHandlerThread(LPVOID lpParam) {
    PrintDebug("Client handler started.");

    ClientHandlerThreadData* threadData = (ClientHandlerThreadData*)lpParam;

    // Access the socket and context
    SOCKET clientSocket = threadData->clientSocket;
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
            PrintDebug("Stop signal received, stopping client handler.");

            break;
        }

        // Receive data from the client
        recvResult = recv(clientSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received: '%s' with length %d.", receiveBuffer, recvResult);

            // Message to reply with
            const char* replyMessage = "Replying to client";

            // Respond to the client
            PrintDebug("Replying to the client.");
            sendResult = send(clientSocket, replyMessage, (int)strlen(replyMessage) + 1, 0);
            if (sendResult > 0) {
                PrintInfo("Reply sent: '%s' with length %d.", replyMessage, sendResult);
            } else if (sendResult == 0) {
                PrintInfo("Client disconnected.");

                break;
            } else {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    // Ignore WSAEWOULDBLOCK, it is not an actual error
                    PrintError("[ClientHandlerThread] 'send' failed with error: %d.", WSAGetLastError());

                    break;
                }

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
        PrintError("[ClientHandlerThread] 'send' failed with error: %d.", WSAGetLastError());
    }

    // Close the client socket
    PrintDebug("Closing client socket.");
    iResult = closesocket(clientSocket);
    if (iResult == SOCKET_ERROR) {
        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup the thread data
        free(threadData);

        return FALSE;
    }

    // Cleanup the thread data
    free(threadData);

    PrintDebug("Client handler stopped.");

    return TRUE;
}