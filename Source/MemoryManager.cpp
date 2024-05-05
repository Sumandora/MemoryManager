#include "MemoryManager/MemoryManager.hpp"

bool MemoryManager::MemoryManager::requiresPermissionsForReading() const {
	return true;
}

bool MemoryManager::MemoryManager::requiresPermissionsForWriting() const {
	return true;
}

using namespace MemoryManager;

ProtectionFlags::ProtectionFlags(std::array<char, 3> permissions) {
	set(0, permissions.at(0) == 'r');
	set(1, permissions.at(1) == 'w');
	set(2, permissions.at(2) == 'x');
}

ProtectionFlags::ProtectionFlags(bool readable, bool writable, bool executable) {
	set(0, readable);
	set(1, writable);
	set(2, executable);
}

bool ProtectionFlags::operator==(const Flags& flags) const {
	return
		isReadable() == flags.isReadable() &&
		isWriteable() == flags.isWriteable() &&
		isExecutable() == flags.isExecutable();
}

Flags::Flags(std::array<char, 4> permissions)
{
	set(0, permissions.at(0) == 'r');
	set(1, permissions.at(1) == 'w');
	set(2, permissions.at(2) == 'x');
	set(3, permissions.at(3) == 'p');
}

Flags::Flags(bool readable, bool writable, bool executable, bool _private) {
	set(0, readable);
	set(1, writable);
	set(2, executable);
	set(3, _private);
}

std::string Flags::asString() const
{
	std::string string;
	string += test(0) ? 'r' : '-';
	string += test(1) ? 'w' : '-';
	string += test(2) ? 'x' : '-';
	string += test(3) ? 'p' : '-';
	return string;
}

bool Flags::operator==(const ProtectionFlags& protectionFlags) const
{
	return
		isReadable() == protectionFlags.isReadable() &&
		isWriteable() == protectionFlags.isWriteable() &&
		isExecutable() == protectionFlags.isExecutable();
}

CachedRegion MemoryRegion::cache() const
{
	CachedRegion region{ beginAddress, length };
	parent->read(beginAddress, region.bytes.get(), length);
	return region;
}

const MemoryRegion* MemoryLayout::findRegion(std::uintptr_t address) const
{
	if(empty())
		return nullptr;
	auto it = upper_bound(address);
	if(it == begin())
		return nullptr;
	it--;
	auto& region = *it;
	if(region.isInside(address))
		return &region;
	return nullptr;
}