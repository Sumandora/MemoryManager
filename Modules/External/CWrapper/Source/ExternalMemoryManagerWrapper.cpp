#include "MemoryManager/ExternalMemoryManager.hpp"
#include "MemoryManager/ExternalMemoryManager.h"

#include <stdexcept>

MemoryManager::ExternalMemoryManager::Mode MemoryManager::convertMode(MemoryManagerMode cMode) {
	switch (cMode) {
	case MEMORYMANAGER_NONE:
		return ExternalMemoryManager::Mode::NONE;
	case MEMORYMANAGER_READ:
		return ExternalMemoryManager::Mode::READ;
	case MEMORYMANAGER_WRITE:
		return ExternalMemoryManager::Mode::WRITE;
	case MEMORYMANAGER_READ_AND_WRITE:
		return ExternalMemoryManager::Mode::READ_AND_WRITE;
	}
	throw std::runtime_error{ std::to_string(cMode) };
}

using namespace MemoryManager;

extern "C" {

size_t emmgr_sizeof_externalmmgr = sizeof(ExternalMemoryManager);

void emmgr_construct_external_from_int(void* externalmemorymanager, size_t pid, enum MemoryManagerMode mode) {
	new (externalmemorymanager) ExternalMemoryManager { pid, convertMode(mode) };
}
void emmgr_construct_external_from_string(void* externalmemorymanager, const char* pid, enum MemoryManagerMode mode) {
	new (externalmemorymanager) ExternalMemoryManager { std::string { pid }, convertMode(mode) };
}

bool emmgr_is_read(const void* externalmemorymanager) {
	return static_cast<const ExternalMemoryManager*>(externalmemorymanager)->isRead();
}
bool emmgr_is_write(const void* externalmemorymanager) {
	return static_cast<const ExternalMemoryManager*>(externalmemorymanager)->isWrite();
}

}