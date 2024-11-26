#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// System libraries

#include <stdio.h>
#include <stdarg.h>

// User-defined constants

// Logging levels

#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR   3

// Set the default logging level, to override it set define LOG_LEVEL before importing the header
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
