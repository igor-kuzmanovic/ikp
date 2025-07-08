#pragma once

#include "../Lib/SharedConfig.h"

#include <conio.h>
#include <stdbool.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "LoggingLib.h"
#include "NetworkLib.h"
#include "Protocol.h"
#include "Config.h"

#define LOGGING_NAMESPACE "CL"

#undef PrintDebug
#undef PrintInfo
#undef PrintWarning
#undef PrintError
#undef PrintCritical

#define PrintDebug(format, ...) PrintDebugFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintInfo(format, ...) PrintInfoFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintWarning(format, ...) PrintWarningFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintError(format, ...) PrintErrorFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)
#define PrintCritical(format, ...) PrintCriticalFunc(LOGGING_NAMESPACE, format, ##__VA_ARGS__)

typedef struct {
    int putSuccessCount;
    int getSuccessCount;
    int putCount;
    int getCount;
    int verificationComplete;
    CRITICAL_SECTION lock;
} TestVerification;

typedef struct {
    CRITICAL_SECTION lock;
    HANDLE finishSignal;
    bool finishFlag;
    SOCKET connectSocket;
    bool pauseSender;
    TestVerification testData;
    int messageCount;
} Context;

int GenerateAndSendClientMessage(SOCKET clientSocket, const int id);
void PrintVerificationSummary(Context* context);
int GetLocalPort(const SOCKET clientSocket);
int GenerateKey(char* key, const int localPort, const DWORD processId, const int id);
int GenerateRandomValue(char* value);

#include "Context.h"
