#include "MemoryManager/LocalMemoryManager.hpp"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>

using namespace MemoryManager;

LocalMemoryManager::LocalMemoryManager(Mode mode)
	: ExternalMemoryManager("self", mode)
{
}

bool LocalMemoryManager::isRead() const
{
	return true;
}

bool LocalMemoryManager::isWrite() const
{
	return true;
}

void* LocalMemoryManager::allocate(std::uintptr_t address, std::size_t size, int protection) const
{
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	void* addr = reinterpret_cast<void*>(address);
	if(addr != nullptr)
		flags |= MAP_FIXED_NOREPLACE;
	return mmap(addr, size, protection, flags, -1, 0);
}

void LocalMemoryManager::deallocate(std::uintptr_t address, std::size_t size) const
{
	int res = munmap(reinterpret_cast<void*>(address), size);

#ifndef LOCALMEMORYMANAGER_DISABLE_EXCEPTIONS
	if(res == -1)
		throw std::runtime_error(strerror(errno));
#endif
}

void LocalMemoryManager::read(std::uintptr_t address, void* content, std::size_t length) const
{
	if (!ExternalMemoryManager::isRead()) {
		std::memcpy(content, reinterpret_cast<void*>(address), length);
		return;
	}

	ExternalMemoryManager::read(address, content, length);
}

void LocalMemoryManager::write(std::uintptr_t address, const void* content, std::size_t length) const
{
	if (!ExternalMemoryManager::isWrite()) {
		std::memcpy(reinterpret_cast<void*>(address), content, length);
		return;
	}

	ExternalMemoryManager::write(address, content, length);
}