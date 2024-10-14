#include "MemoryManager/LinuxMemoryManager.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main()
{
	MemoryManager::LinuxMemoryManager<true, true, true> memoryManager;

	int* myInteger = reinterpret_cast<int*>(memoryManager.allocate(0, sizeof(int), "---" /*No permissions*/));
	memoryManager.update();

	std::cout << std::hex;
	for (const auto& reg : memoryManager.getLayout()) {
		auto name = reg.getName().value_or("unnamed");
		auto path = reg.getPath().value_or("pathless");
		std::cout << reg.getAddress() << '-' << reg.getAddress() + reg.getLength() << ' ' << reg.getFlags().asString() << ' ' << path << " (" << name << ")\n";
	}

	std::cout << "Allocated memory at " << myInteger << '\n';

	auto integerPtr = reinterpret_cast<std::uintptr_t>(myInteger);

	const auto* region = memoryManager.getLayout().findRegion(integerPtr);
	assert(region != nullptr);
	const auto* sameRegion = memoryManager.getLayout().findRegion(region->getAddress());
	assert(region == sameRegion);

	std::cout << "Page region: " << region->getAddress() << "-" << region->getAddress() + region->getLength() << '\n';

	std::cout << std::dec;

	auto span = region->view();

	std::uint64_t s = 0;
	for(std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 0);


	int val = -1;
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "Before writing: " << val << '\n';
	val = 123;
	memoryManager.write(integerPtr, &val, sizeof(int));
	val = -1; // Write some garbage to it, to prevent the read function from cheating
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "After writing: " << val << '\n';
	assert(val == 123);

	span = region->view(true);
	s = 0;
	for(std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 123);

	memoryManager.deallocate(reinterpret_cast<std::uintptr_t>(myInteger), sizeof(int));

	return 0;
}
