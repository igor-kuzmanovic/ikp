#include "LoggingLib.h"

static void PrintFormatted(const char* level, const char* namespc, const char* format, va_list args) {
    char timeBuffer[64];
    char messageBuffer[1024];
    struct timeb timeData;
    struct tm timeInfo;

    ftime(&timeData);
    if (localtime_s(&timeInfo, &timeData.time)) {
        snprintf(timeBuffer, sizeof(timeBuffer), "UNKNOWN TIME");
    } else {
        snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,
            timeData.millitm);
    }

    if (namespc && namespc[0] != '\0') {
        snprintf(messageBuffer, sizeof(messageBuffer), "[%s][%s][%s] %s\n", timeBuffer, namespc, level, format);
    } else {
        snprintf(messageBuffer, sizeof(messageBuffer), "[%s][%s] %s\n", timeBuffer, level, format);
    }

    vprintf(messageBuffer, args);
}

void PrintDebugFunc(const char* namespc, const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    va_list args;
    va_start(args, format);
    PrintFormatted("DBG", namespc, format, args);
    va_end(args);
#endif
}

void PrintInfoFunc(const char* namespc, const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_INFO
    va_list args;
    va_start(args, format);
    PrintFormatted("INF", namespc, format, args);
    va_end(args);
#endif
}

void PrintWarningFunc(const char* namespc, const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_WARNING
    va_list args;
    va_start(args, format);
    PrintFormatted("WRN", namespc, format, args);
    va_end(args);
#endif
}

void PrintErrorFunc(const char* namespc, const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_ERROR
    va_list args;
    va_start(args, format);
    PrintFormatted("ERR", namespc, format, args);
    va_end(args);
#endif
}

void PrintCriticalFunc(const char* namespc, const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_CRITICAL
    va_list args;
    va_start(args, format);
    PrintFormatted("CRT", namespc, format, args);
    va_end(args);
#endif
}


