#ifndef MEMORYMANAGER_EXTERNALMEMORYMANAGER_H
#define MEMORYMANAGER_EXTERNALMEMORYMANAGER_H

#include "MemoryManager/MemoryManager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MemoryManagerMode {
	MEMORYMANAGER_NONE,
	MEMORYMANAGER_READ,
	MEMORYMANAGER_WRITE,
	MEMORYMANAGER_READ_AND_WRITE
};

#ifdef __cplusplus
namespace MemoryManager {
	ExternalMemoryManager::Mode convertMode(MemoryManagerMode cMode);
}
#endif

extern size_t mmgr_sizeof_externalmmgr;

void mmgr_construct_external_from_int(void* externalmemorymanager, size_t pid, enum MemoryManagerMode mode);
void mmgr_construct_external_from_string(void* externalmemorymanager, const char* pid, enum MemoryManagerMode mode);

bool emmgr_is_read(const void* externalmemorymanager);
bool emmgr_is_write(const void* externalmemorymanager);

#ifdef __cplusplus
}
#endif

#endif
