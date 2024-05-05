#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;
using CacheIterator = CachedRegion::CacheIterator;

CachedRegion::CachedByte CacheIterator::operator*() const {
	return (*parent)[index];
}

std::byte* CacheIterator::operator->() const {
	return &(*parent)[index];
}


CacheIterator& CacheIterator::operator++() {
	index++;
	return *this;
}

CacheIterator CacheIterator::operator++(int) {
	CacheIterator it = *this;
	index++;
	return it;
}


CacheIterator& CacheIterator::operator--() {
	index--;
	return *this;
}

CacheIterator CacheIterator::operator--(int) {
	CacheIterator it = *this;
	index--;
	return it;
}


bool CacheIterator::operator==(const CacheIterator& rhs) const {
	return index == rhs.index;
}