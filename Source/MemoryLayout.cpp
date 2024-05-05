#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;

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