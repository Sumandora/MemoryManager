#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;

CachedRegion::CachedRegion(std::uintptr_t remoteAddress, std::size_t length)
	: remoteAddress(remoteAddress)
	, length(length)
	, bytes(std::unique_ptr<std::byte[]>{ new std::byte[length] })
{
}


[[nodiscard]] CachedRegion::CachedByte CachedRegion::operator[](std::size_t i) const {
	return { bytes[i], reinterpret_cast<std::byte*>(remoteAddress + i) };
}

[[nodiscard]] CachedRegion::CacheIterator CachedRegion::cbegin() const {
	return { this, 0 };
}
[[nodiscard]] CachedRegion::CacheIterator CachedRegion::cend() const {
	return { this, getLength() };
}

[[nodiscard]] std::reverse_iterator<CachedRegion::CacheIterator> CachedRegion::crbegin() const {
	return std::make_reverse_iterator(cend());
}
[[nodiscard]] std::reverse_iterator<CachedRegion::CacheIterator> CachedRegion::crend() const {
	return std::make_reverse_iterator(cbegin());
}
