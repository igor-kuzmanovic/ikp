#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>

// User-defined constants

// Logging levels

#define LOG_LEVEL_DEBUG     0
#define LOG_LEVEL_INFO      1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_ERROR     3
#define LOG_LEVEL_CRITICAL  4

// Set the default logging level, to override it set define LOG_LEVEL before importing the header
// This doesn't quite work at the moment but might be useful if we figure out how to override it per project
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// API

// Functions

// Helper function to print debug messages
void PrintDebug(const char* message, ...);

// Helper function to print info messages
void PrintInfo(const char* message, ...);

// Helper function to print warning messages
void PrintWarning(const char* message, ...);

// Helper function to print error messages
void PrintError(const char* message, ...);

// Helper function to print critical messages
void PrintCritical(const char* message, ...);
