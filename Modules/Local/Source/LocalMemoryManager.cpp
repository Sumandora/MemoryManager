#include "MemoryManager/LocalMemoryManager.hpp"

#include <cstring>
#include <sstream>
#include <sys/mman.h>

using namespace MemoryManager;

LocalMemoryManager::LocalMemoryManager(Mode forceMode)
	: ExternalMemoryManager("self", forceMode)
{
}

bool LocalMemoryManager::doesRead() const
{
	return true;
}

bool LocalMemoryManager::doesWrite() const
{
	return true;
}

bool LocalMemoryManager::doesForceRead() const
{
	return ExternalMemoryManager::doesRead();
}

bool LocalMemoryManager::doesForceWrite() const
{
	return ExternalMemoryManager::doesWrite();
}

static int protectionFlagsToPosix(ProtectionFlags flags) {
	int prot = 0;

	if(flags.isReadable())
		prot |= PROT_READ;
	if(flags.isWriteable())
		prot |= PROT_WRITE;
	if(flags.isExecutable())
		prot |= PROT_EXEC;

	return prot;
}

std::uintptr_t LocalMemoryManager::allocate(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const
{
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	void* addr = reinterpret_cast<void*>(address);
	if(addr != nullptr)
		flags |= MAP_FIXED_NOREPLACE;
	void* res = mmap(addr, size, protectionFlagsToPosix(protection), flags, -1, 0);
	if(res == MAP_FAILED)
		throw std::runtime_error(strerror(errno));
	return reinterpret_cast<uintptr_t>(res);
}

void LocalMemoryManager::deallocate(std::uintptr_t address, std::size_t size) const
{
	int res = munmap(reinterpret_cast<void*>(address), size);

	if(res == -1)
		throw std::runtime_error(strerror(errno));
}

void LocalMemoryManager::protect(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const {
	int res = mprotect(reinterpret_cast<void*>(address), size, protectionFlagsToPosix(protection));

	if(res == -1)
		throw std::runtime_error(strerror(errno));
}

void LocalMemoryManager::read(std::uintptr_t address, void* content, std::size_t length) const
{
	if (!ExternalMemoryManager::doesRead()) {
		std::memcpy(content, reinterpret_cast<void*>(address), length);
		return;
	}

	ExternalMemoryManager::read(address, content, length);
}

void LocalMemoryManager::write(std::uintptr_t address, const void* content, std::size_t length) const
{
	if (!ExternalMemoryManager::doesWrite()) {
		std::memcpy(reinterpret_cast<void*>(address), content, length);
		return;
	}

	ExternalMemoryManager::write(address, content, length);
}

bool LocalMemoryManager::requiresPermissionsForReading() const {
	return !doesForceRead();
}

bool LocalMemoryManager::requiresPermissionsForWriting() const {
	return !doesForceWrite();
}