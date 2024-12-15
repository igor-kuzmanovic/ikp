#include "ClientHandlerThread.h"

DWORD WINAPI ClientHandlerThread(LPVOID lpParam) {
    ClientHandlerThreadData* threadData = (ClientHandlerThreadData*)lpParam;

    // Access the socket and context
    SOCKET clientSocket = threadData->clientSocket;
    LoadBalancerContext* ctx = threadData->ctx;

    int iResult;

    // Buffer used for storing incoming data
    char receiveBuffer[BUFFER_SIZE]{};

    // A variable to store the result of recv
    int recvResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    while (true) {
        // Check stop signal
        if (ctx->stopServer) {
            PrintInfo("Stop signal received.");

            break;
        }

        // Receive data from the client
        recvResult = recv(clientSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received: '%s' with length %d.", receiveBuffer, recvResult);

            // Message to reply with
            const char* messageToReply = "This is a test reply";

            // Respond to the client
            PrintDebug("Replying to the client.");
            sendResult = send(clientSocket, messageToReply, (int)strlen(messageToReply) + 1, 0);
            if (sendResult == SOCKET_ERROR) {
                PrintError("'send' failed with error: %d.", WSAGetLastError());

                break;
            }
        } else if (recvResult == 0) {
            PrintInfo("Client disconnected.");
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }
        }

        Sleep(10); // Avoid busy waiting
    }

    // Send shutdown notification
    if (ctx->stopServer) {
        PrintDebug("Notifying client of server shutdown.");
        const char* shutdownMessage = "Server is shutting down.";
        sendResult = send(clientSocket, shutdownMessage, (int)strlen(shutdownMessage) + 1, 0);
        if (sendResult == SOCKET_ERROR) {
            PrintError("'send' failed with error: %d.", WSAGetLastError());
        }
    }

    // Close the client socket
    PrintDebug("Closing client socket.");
    iResult = closesocket(clientSocket);
    if (iResult == SOCKET_ERROR) {
        PrintError("'closesocket' failed with error: %d.", WSAGetLastError());

        // Cleanup the thread data
        free(threadData);

        return EXIT_FAILURE;
    }

    // Cleanup the thread data
    free(threadData);

    return EXIT_SUCCESS;
}