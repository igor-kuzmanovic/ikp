#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    Context* context = (Context*)lpParam;

    char receiveBuffer[BUFFER_SIZE]{};
    int recvResult = 0;

    KeyValuePair kvp = {};

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping receiver.");

            break;
        }

        // Receive data from server
        recvResult = recv(context->connectSocket, receiveBuffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            // Check if server is shutting down
            if (strstr(receiveBuffer, SERVER_SHUTDOWN_MESSAGE) != NULL) {
                PrintInfo("Server shutdown notification received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(context);

                break;
            } else if (strstr(receiveBuffer, WORKER_HEALTH_CHECK_MESSAGE) != NULL) {
                PrintInfo("Health check request received.");

                continue;
            } else {
                PrintInfo("Client request received: '%s' with length %d.", receiveBuffer, recvResult);

                if (DeserializeKVPair(receiveBuffer, &kvp) != 0) {
                    PrintError("Deserialization failed.");

                    continue;
                }

                SetHashTable(context->hashTable, kvp.key, kvp.value);
            }
        } else if (recvResult == 0) {
            PrintInfo("Server closed the connection.");

            break;
        } else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                // Ignore WSAEWOULDBLOCK, it is not an actual error
                PrintError("'recv' failed with error: %d.", WSAGetLastError());

                break;
            }

            Sleep(INPUT_WAIT_TIME);

            continue;
        }
    };

    PrintDebug("Receiver stopped.");

    return TRUE;
}