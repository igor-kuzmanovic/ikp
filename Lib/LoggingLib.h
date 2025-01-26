#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>

// User-defined constants

// Logging levels

#define LOG_LEVEL_DEBUG     0x0
#define LOG_LEVEL_INFO      0x1
#define LOG_LEVEL_WARNING   0x2
#define LOG_LEVEL_ERROR     0x3
#define LOG_LEVEL_CRITICAL  0x4

// Set the default logging level, to override it set define LOG_LEVEL before importing the header
// This doesn't quite work at the moment but might be useful if we figure out how to override it per project
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// API

// Functions

// Helper function to print debug messages
void PrintDebug(const char* namespc, const char* message, ...);

// Helper function to print info messages
void PrintInfo(const char* namespc, const char* message, ...);

// Helper function to print warning messages
void PrintWarning(const char* namespc, const char* message, ...);

// Helper function to print error messages
void PrintError(const char* namespc, const char* message, ...);

// Helper function to print critical messages
void PrintCritical(const char* namespc, const char* message, ...);
