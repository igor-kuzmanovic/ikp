#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    PrintDebug("Receiver started.");

    // Context
    Context* context = (Context*)lpParam;

    // Buffer used for storing incoming data
    MessageBuffer receiveMessageBuffer{};

    // A variable to store the result of recv
    int recvResult = 0;

    // A variable to exponentially increase the sleep time when server is busy
    int serverFullSleepTimeMultiplier = 1;

    while (true) {
        // Wait for the signal to stop the thread
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            PrintDebug("Stop signal received, stopping receiver.");

            break;
        }

        // Receive data from server
        recvResult = recv(context->connectSocket, receiveMessageBuffer.buffer, BUFFER_SIZE, 0);
        if (recvResult > 0) {
            PrintInfo("Message received with length %d.", recvResult);

            switch (receiveMessageBuffer.message.type) {
            case MSG_KEY_VALUE_PAIR_STORED:
                // TODO Remove worker id print later, client doesn't need to know about it
                PrintInfo("Key-Value Pair with key '%s' stored on worker id %d.", receiveMessageBuffer.message.payload.keyValuePairStored.key, receiveMessageBuffer.message.payload.keyValuePairStored.workerId);

                break;
            case MSG_SERVER_SHUTDOWN:
                PrintInfo("Server shutdown notification received.");

                PrintDebug("Setting the finish signal.");
                SetFinishSignal(context);

                break;
            case MSG_SERVER_BUSY:
                PrintInfo("Server is busy, pausing the sender.");

                PrintDebug("Enabling the pause sender flag.");
                SetPauseSender(context, true);

                PrintDebug("Sleeping for %d ms before unpausing the sender.", SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);
                Sleep(SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);

                PrintDebug("Increasing the sleep time multiplier.");
                serverFullSleepTimeMultiplier *= 2;

                PrintDebug("Disabling the pause sender flag.");
                SetPauseSender(context, false);

                PrintInfo("Assuming the server is ready to receive new requests, resuming the sender.");

                continue;
            default:
                PrintError("Unsupported message type: %d.", receiveMessageBuffer.message.type);
                
                continue;
            }

            // Check if server is ready
            if (serverFullSleepTimeMultiplier != 1) {
                PrintDebug("Resetting the sleep time multiplier.");
                serverFullSleepTimeMultiplier = 1;

                continue;
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

            continue;
        }
    };

    PrintDebug("Receiver stopped.");

    return TRUE;
}