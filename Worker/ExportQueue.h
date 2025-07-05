#pragma once

#include "SharedLibs.h"

int InitializeExportQueue(ExportQueue** queue);
int DestroyExportQueue(ExportQueue* queue);
int AddExportRequest(ExportQueue* queue, uint32_t targetWorkerId, const char* address, uint16_t port);
