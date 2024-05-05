#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;


CachedRegion MemoryRegion::cache() const
{
	CachedRegion region{ beginAddress, length };
	parent->read(beginAddress, region.bytes.get(), length);
	return region;
}