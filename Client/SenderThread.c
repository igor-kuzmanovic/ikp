#include "SenderThread.h"
#include "../Lib/Protocol.h"

DWORD WINAPI SenderThread(LPVOID lpParam) {
    Context* context = (Context*)lpParam;

    int messageCounter = 0;
    int sendResult = 0;
    int verificationPhase = 0;
    int verificationCounter = 0;
    int serverFullSleepTimeMultiplier = 1;
    srand((unsigned int)time(NULL));
    int localPort = GetLocalPort(context->connectSocket);
    DWORD processId = GetCurrentProcessId();
    char key[MAX_KEY_SIZE + 1];
    int keyLength;
    char value[MAX_VALUE_SIZE + 1];
    int valueLength;

    PrintInfo("Sending %d messages to the server for PUT/GET verification.", context->messageCount);

    while (true) {
        if (WaitForSingleObject(context->finishSignal, 0) == WAIT_OBJECT_0) {
            break;
        }

        if (context->connectSocket == INVALID_SOCKET) {
            SetFinishSignal(context);
            break;
        }

        if (GetPauseSender(context)) {
            Sleep(SERVER_FULL_SLEEP_TIME * serverFullSleepTimeMultiplier);
            serverFullSleepTimeMultiplier *= 2;
            continue;
        } else if (serverFullSleepTimeMultiplier != 1) {
            serverFullSleepTimeMultiplier = 1;
        }

        if (verificationPhase == 0 && messageCounter >= context->messageCount) {
            PrintInfo("PUT phase complete. Waiting briefly before starting GET verification phase...");
            Sleep(PUT_TO_GET_TRANSITION_DELAY);
            verificationPhase = 1;
            continue;
        }

        if (verificationPhase == 1) {
            if (verificationCounter >= context->messageCount) {
                PrintInfo("GET verification phase complete. All GET requests sent.");
                ReleaseSemaphore(context->getAllRequestsSentSignal, 1, NULL);

                PrintInfo("Waiting for all responses to be received...");
                WaitForSingleObject(context->verificationCompleteSignal, INFINITE);

                PrintVerificationSummary(context);
                break;
            }

            keyLength = GenerateKey(key, localPort, processId, verificationCounter + 1);
            if (keyLength > 0 && keyLength <= MAX_KEY_SIZE) {
                sendResult = SendGet(context->connectSocket, key);
                if (sendResult > 0) {
                    PrintDebug("Verification GET request #%d sent for key '%s'", verificationCounter + 1, key);
                    verificationCounter++;
                } else if (sendResult == 0) {
                    PrintInfo("Server disconnected during verification.");
                    break;
                } else {
                    PrintError("Failed to send verification GET request #%d.", verificationCounter + 1);
                }
            } else {
                PrintWarning("Failed to regenerate key for verification #%d", verificationCounter + 1);
                verificationCounter++;
            }
        } else {
            keyLength = GenerateKey(key, localPort, processId, messageCounter + 1);
            if (keyLength <= 0 || keyLength > MAX_KEY_SIZE) {
                PrintError("Failed to generate a valid key, length: %d.", keyLength);
                continue;
            }

            valueLength = GenerateRandomValue(value);
            if (valueLength <= 0 || valueLength > MAX_VALUE_SIZE) {
                PrintError("Failed to generate a valid value, length: %d.", valueLength);
                continue;
            }

            sendResult = SendPut(context->connectSocket, key, value);
            if (sendResult > 0) {
                PrintDebug("PUT message #%d sent successfully for key '%s'", messageCounter + 1, key);
                EnterCriticalSection(&context->testData.lock);
                context->testData.putCount++;
                context->testData.putSuccessCount++;
                LeaveCriticalSection(&context->testData.lock);
                messageCounter++;
            } else if (sendResult == 0) {
                PrintInfo("Server disconnected.");
                break;
            } else {
                PrintError("Failed to send PUT message #%d (error: %d).", messageCounter + 1, sendResult);
            }
        }

        Sleep(MESSAGE_SEND_WAIT_TIME);
    }

    return TRUE;
}


