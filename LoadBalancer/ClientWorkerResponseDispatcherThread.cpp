#include "ClientWorkerResponseDispatcherThread.h"

DWORD WINAPI ClientWorkerResponseDispatcherThread(LPVOID lpParam) {
    PrintDebug("Client worker response dispatcher started.");

    Context* context = (Context*)lpParam;

    int iResult = 0;

    // A variable to store the result of send
    int sendResult = 0;

    // A variable to store the worker response
    WorkerResponse* response = (WorkerResponse*)malloc(sizeof(WorkerResponse));
    if (response == NULL) {
        PrintError("Failed to allocate memory for the worker response.");

        return FALSE;
    }

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping client worker response dispatcher.");

            break;
        }

        iResult = TakeWorkerResponseQueue(context->workerResponseQueue, response);
        if (iResult == 0) {
            Sleep(WORKER_RESPONSE_QUEUE_EMPTY_SLEEP_TIME);

            continue;
        } else if (iResult < 0) {
            PrintError("Failed to take the worker response.");

            memset(response, 0, sizeof(WorkerResponse));

            continue;
        } else {
            PrintDebug("Got a new worker response.");
        }

        PrintDebug("Sending response to client '%s'.",  response->data);

        // TODO Route the response with a client socket
        //sendResult = send(client->socket, response->data, (int)strlen(response->data) + 1, 0);
        //if (sendResult > 0) {
        //    PrintInfo("Message sent to client %d: '%s' with length %d.", client->socket, response->data, sendResult);

        //    memset(response, 0, sizeof(WorkerResponse));
        //} else if (sendResult == 0) {
        //    PrintError("Client disconnected.");
        //} else {
        //    if (WSAGetLastError() != WSAEWOULDBLOCK) {
        //        // Ignore WSAEWOULDBLOCK, it is not an actual error
        //        PrintError("[ClientWorkerResponseDispatchedThread] 'send' failed with error: %d.", WSAGetLastError());
        //    }

        //    continue;
        //}
    }

    free(response);

    PrintDebug("Client worker response dispatcher stopped.");

    return TRUE;
}
