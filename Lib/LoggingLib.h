#pragma once

#include "SharedConfig.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>

#define LOG_LEVEL_DEBUG     0x0
#define LOG_LEVEL_INFO      0x1
#define LOG_LEVEL_WARNING   0x2
#define LOG_LEVEL_ERROR     0x3
#define LOG_LEVEL_CRITICAL  0x4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

void PrintDebugFunc(const char* namespc, const char* message, ...);
void PrintInfoFunc(const char* namespc, const char* message, ...);
void PrintWarningFunc(const char* namespc, const char* message, ...);
void PrintErrorFunc(const char* namespc, const char* message, ...);
void PrintCriticalFunc(const char* namespc, const char* message, ...);
