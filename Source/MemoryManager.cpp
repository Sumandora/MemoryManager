#include "MemoryManager/MemoryManager.hpp"
#include <iostream>

using namespace MemoryManager;

Pointer::Pointer(const MemoryManager* parent, std::uintptr_t address)
: parent(parent)
, address(address)
{
}

void Pointer::write(const void* content, std::size_t length)
{
	return parent->write(address, content, length);
}

void Pointer::read(void* content, std::size_t length)
{
	return parent->read(address, content, length);
}

MemoryRegionFlags::MemoryRegionFlags(std::array<char, 4> permissions)
{
	if(permissions.at(0) == 'r')
		set(0, true);
	if(permissions.at(1) == 'w')
		set(1, true);
	if(permissions.at(2) == 'x')
		set(2, true);
	if(permissions.at(3) == 'p')
		set(3, true);
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
}