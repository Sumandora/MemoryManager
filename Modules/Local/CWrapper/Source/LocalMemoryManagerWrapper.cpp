#include "MemoryManager/LocalMemoryManager.hpp"
#include "MemoryManager/LocalMemoryManager.h"

using namespace MemoryManager;

extern "C" {

size_t mmgr_sizeof_localmmgr = sizeof(LocalMemoryManager);

void mmgr_construct_local(void* localmemorymanager, enum MemoryManagerMode mode) {
	new (localmemorymanager) LocalMemoryManager { convertMode(mode) };
}

}