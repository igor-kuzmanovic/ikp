#include "LoggingLib.h"

static void PrintFormatted(const char* level, const char* format, va_list args) {
    char timeBuffer[64];       // Buffer to hold the formatted date-time string
    char messageBuffer[1024];  // Buffer to hold the full message
    struct timeb timeData;     // To get current time including milliseconds
    struct tm timeInfo;        // Thread-safe localtime_s result buffer

    // Get the current time
    ftime(&timeData); // Get time with millisecond precision
    if (localtime_s(&timeInfo, &timeData.time)) {
        // Handle error in case localtime_s fails
        snprintf(timeBuffer, sizeof(timeBuffer), "UNKNOWN TIME");
    } else {
        // Format the date-time string: "YYYY-MM-DD HH:MM:SS.mmm"
        snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, // Date
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,  // Time
            timeData.millitm);  // Milliseconds
    }

    // Combine the date-time, level, and user message
#ifdef LOG_NAMESPACE
    snprintf(messageBuffer, sizeof(messageBuffer), "[%s] [%s] [%s] %s\n", timeBuffer, LOG_NAMESPACE, level, format);
#else
    snprintf(messageBuffer, sizeof(messageBuffer), "[%s] [%s] %s\n", timeBuffer, level, format);
#endif

    // Print the formatted string with arguments
    vprintf(messageBuffer, args);
}

void PrintDebug(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    va_list args;
    va_start(args, format);
    PrintFormatted("DEBUG", format, args);
    va_end(args);
#endif
}

void PrintInfo(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_INFO
    va_list args;
    va_start(args, format);
    PrintFormatted("INFO", format, args);
    va_end(args);
#endif
}

void PrintWarning(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_WARNING
    va_list args;
    va_start(args, format);
    PrintFormatted("WARNING", format, args);
    va_end(args);
#endif
}

void PrintError(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_ERROR
    va_list args;
    va_start(args, format);
    PrintFormatted("ERROR", format, args);
    va_end(args);
#endif
}

void PrintCritical(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_CRITICAL
    va_list args;
    va_start(args, format);
    PrintFormatted("CRITICAL", format, args);
    va_end(args);
#endif
}
