#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_H
#define MEMORYMANAGER_LOCALMEMORYMANAGER_H

#include "MemoryManager/ExternalMemoryManager.h"

#ifdef __cplusplus
extern "C" {
#endif

extern size_t mmgr_sizeof_localmmgr;

void mmgr_construct_local(void* localmemorymanager, enum MemoryManagerMode mode);

#ifdef __cplusplus
}
#endif

#endif
