#include "MemoryManager/LinuxMemoryManager.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
	MemoryManager::LinuxMemoryManager<true, true, true> memoryManager;

	int* myInteger = reinterpret_cast<int*>(memoryManager.allocate(0, sizeof(int), { false, false, false } /*No permissions*/));
	memoryManager.update();

	std::cout << std::hex;
	for (const auto& reg : memoryManager.getLayout()) {
		auto name = reg.getName().value_or("unnamed");
		auto path = reg.getPath().value_or("pathless");
		std::cout << path << " (" << name << ") " << reg.getFlags().asString() << ' ' << reg.getAddress() << '-' << reg.getAddress() + reg.getLength() << '\n';
	}

	std::cout << "Allocated memory at " << myInteger << '\n';

	auto integerPtr = reinterpret_cast<std::uintptr_t>(myInteger);

	const auto* region = memoryManager.getLayout().findRegion(integerPtr);
	assert(region != nullptr);
	const auto* sameRegion = memoryManager.getLayout().findRegion(region->getAddress());
	assert(region == sameRegion);

	std::cout << "Page region: " << region->getAddress() << "-" << region->getAddress() + region->getLength() << '\n';

	std::cout << std::dec;

	int val = 0;
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "Before writing: " << val << '\n';
	val = 123;
	memoryManager.write(integerPtr, &val, sizeof(int));
	val = 1337;
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "After writing: " << val << '\n';
	assert(val == 123);

	return 0;
}
