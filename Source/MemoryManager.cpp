#include "MemoryManager/MemoryManager.hpp"
#include <iostream>

using namespace MemoryManager;

Pointer::Pointer(const MemoryManager* parent, std::uintptr_t address)
: parent(parent)
, address(address)
{
}

void Pointer::write(const void* content, std::size_t length) const
{
	return parent->write(address, content, length);
}

void Pointer::read(void* content, std::size_t length) const
{
	return parent->read(address, content, length);
}

Flags::Flags(std::array<char, 4> permissions)
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
}