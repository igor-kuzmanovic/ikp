#include "LoggingLib.h"

static void PrintFormatted(const char* prefix, const char* format, va_list args) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s %s\n", prefix, format);
    vprintf(buffer, args);
}

void PrintDebug(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    va_list args;
    va_start(args, format);
    PrintFormatted("[DEBUG]", format, args);
    va_end(args);
#endif
}

void PrintInfo(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_INFO
    va_list args;
    va_start(args, format);
    PrintFormatted("[INFO]", format, args);
    va_end(args);
#endif
}

void PrintWarning(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_WARNING
    va_list args;
    va_start(args, format);
    PrintFormatted("[WARNING]", format, args);
    va_end(args);
#endif
}

void PrintError(const char* format, ...) {
#if LOG_LEVEL <= LOG_LEVEL_ERROR
    va_list args;
    va_start(args, format);
    PrintFormatted("[ERROR]", format, args);
    va_end(args);
#endif
}
