#include "MemoryManager/MemoryManager.hpp"
#include <iostream>

bool MemoryManager::MemoryManager::requiresPermissionsForReading() const {
	return true;
}

bool MemoryManager::MemoryManager::requiresPermissionsForWriting() const {
	return true;
}

using namespace MemoryManager;

ProtectionFlags::ProtectionFlags(bool readable, bool writable, bool executable) {
	set(0, readable);
	set(1, writable);
	set(2, executable);
}

Flags::Flags(std::array<char, 4> permissions)
{
	set(0, permissions.at(0) == 'r');
	set(1, permissions.at(1) == 'w');
	set(2, permissions.at(2) == 'x');
	set(3, permissions.at(3) == 'p');
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