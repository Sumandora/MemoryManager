#include "MemoryManager/LocalMemoryManager.hpp"

#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <fcntl.h>
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