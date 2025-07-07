#include "ReceiverThread.h"

DWORD WINAPI ReceiverThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    MessageType messageType;
    uint16_t actualSize;
    int recvResult = 0;
    int serverFullSleepTimeMultiplier = 1;
    int errorCount = 0;
    char buffer[MAX_MESSAGE_SIZE];
    char key[MAX_KEY_SIZE + 1];
    char value[MAX_VALUE_SIZE + 1];
    ErrorCode errorCode;
    char errorMessage[MAX_ERROR_MSG_SIZE + 1];

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        if (context->connectSocket == INVALID_SOCKET) {
            SetFinishSignal(context);
            break;
        }

        recvResult = ProtocolReceive(context->connectSocket, &messageType, buffer, MAX_MESSAGE_SIZE, &actualSize);
        if (recvResult == 0) {
            PrintDebug("Message received: %s (%d bytes)", GetMessageTypeName(messageType), actualSize);

            switch (messageType) {
            case MSG_PUT_RESPONSE: {
                if (ReceivePutResponse(buffer, actualSize, &errorCode, key) == 0) {
                    EnterCriticalSection(&context->testData.lock);
                    if (errorCode == ERR_NONE) {
                        PrintDebug("PUT request for key '%s' was successful.", key);
                        context->testData.putSuccessCount++;
                    } else {
                        PrintWarning("PUT request for key '%s' failed: %s", key, GetErrorCodeName(errorCode));
                    }
                    LeaveCriticalSection(&context->testData.lock);
                } else {
                    PrintError("Failed to parse PUT response message");
                }
                break;
            }

            case MSG_GET_RESPONSE: {
                if (ReceiveGetResponse(buffer, actualSize, &errorCode, key, value) == 0) {
                    EnterCriticalSection(&context->testData.lock);
                    if (errorCode == ERR_NONE) {
                        PrintDebug("GET request for key '%s' returned value: '%s'", key, value);
                        context->testData.getSuccessCount++;
                    } else {
                        PrintWarning("GET request for key '%s' failed: %s", key, GetErrorCodeName(errorCode));
                    }
                    if (context->testData.getCount >= context->messageCount) {
                        ReleaseSemaphore(context->verificationCompleteSignal, 1, NULL);
                    }
                    LeaveCriticalSection(&context->testData.lock);
                } else {
                    PrintError("Failed to parse GET response message");
                }
                break;
            }

            case MSG_SHUTDOWN:
                PrintInfo("Server shutdown notification received.");
                SetFinishSignal(context);
                break;

            case MSG_ERROR: {
                if (ReceiveError(buffer, actualSize, &errorCode, errorMessage) == 0) {
                    PrintError("Server error (%s): %s", GetErrorCodeName(errorCode), errorMessage);
                    if (errorCode == ERR_SERVER_BUSY) {
                        PrintDebug("Server is busy, pausing the sender.");
                        SetPauseSender(context, true);
                        Sleep(CLIENT_SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);
                        serverFullSleepTimeMultiplier *= 2;
                        SetPauseSender(context, false);
                        PrintDebug("Assuming the server is ready to receive new requests, resuming the sender.");
                    }
                } else {
                    PrintError("Failed to parse error message");
                }
                break;
            }
            }

            if (serverFullSleepTimeMultiplier != 1) {
                serverFullSleepTimeMultiplier = 1;
            }
        } else {
            if (recvResult == -2 || recvResult == -3) {
                PrintInfo("Server disconnected or network error (error: %d). Closing connection gracefully.", recvResult);
            } else {
                PrintError("'ProtocolReceive' failed with error: %d. Closing connection.", recvResult);
            }
            SetFinishSignal(context);
            break;
        }
    }

    return TRUE;
}


