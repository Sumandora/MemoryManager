#include "MemoryManager/LinuxMemoryManager.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <print>

int main()
{
	MemoryManager::LinuxMemoryManager<true, true, true> memoryManager;

	std::uintptr_t myInteger = memoryManager.allocate(sizeof(int), "---" /*No permissions*/);
	memoryManager.update();

	for (const auto& reg : memoryManager.getLayout()) {
		auto name = reg.getName().value_or("unnamed");
		auto path = reg.getPath().value_or("pathless");
		std::println("{:x}-{:x} {:s} {:s} ({:s})",
			reg.getAddress(),
			reg.getAddress() + reg.getLength(),
			reg.getFlags().toString(),
			path,
			name);
	}

	std::println("Allocated memory at {:#x}", myInteger);

	const auto* region = memoryManager.getLayout().findRegion(myInteger);
	assert(region != nullptr);
	const auto* sameRegion = memoryManager.getLayout().findRegion(region->getAddress());
	assert(region == sameRegion);

	std::println("Page region: {:#x}-{:#x}", region->getAddress(), region->getAddress() + region->getLength());

	auto span = region->view();

	std::uint64_t s = 0;
	for (const std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 0);

	int val = -1;
	memoryManager.read(myInteger, &val, sizeof(int));
	std::println("Before writing: {}", val);
	val = 123;
	memoryManager.write(myInteger, &val, sizeof(int));
	val = -1; // Write some garbage to it, to prevent the read function from cheating
	memoryManager.read(myInteger, &val, sizeof(int));
	std::println("After writing: {}", val);
	assert(val == 123);

	span = region->view(true);
	s = 0;
	for (const std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 123);

	memoryManager.deallocate(myInteger, sizeof(int));

	return 0;
}
