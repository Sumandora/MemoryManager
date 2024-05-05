#include "MemoryManager/MemoryManager.hpp"

// Don't want to put this into either of the cpp files, so just keep it here as it's seen as global inside the namespace
bool MemoryManager::operator==(const Flags& flags, const ProtectionFlags& protectionFlags)
{
	return
		flags.isReadable() == protectionFlags.isReadable() &&
		flags.isWriteable() == protectionFlags.isWriteable() &&
		flags.isExecutable() == protectionFlags.isExecutable();
}

bool MemoryManager::MemoryManager::requiresPermissionsForReading() const {
	return true;
}

bool MemoryManager::MemoryManager::requiresPermissionsForWriting() const {
	return true;
}