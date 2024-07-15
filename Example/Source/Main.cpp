#include "MemoryManager/LinuxMemoryManager.hpp"

#include <cassert>
#include <iostream>

int main()
{
	MemoryManager::LinuxMemoryManager<true, true, true> memoryManager;

	int* myInteger = reinterpret_cast<int*>(memoryManager.allocate(0, sizeof(int), { false, false, false } /*No permissions*/));
	memoryManager.update();

	std::cout << std::hex;
	for (const auto& reg : memoryManager.getLayout()) {
		std::cout << (reg.getName().has_value() ? reg.getName().value() : std::string("(empty)")) << ' ' << reg.getFlags().asString() << ' ' << reg.getAddress() << '-' << reg.getAddress() + reg.getLength() << std::endl;
	}

	std::cout << "Allocated memory at " << myInteger << std::endl;

	auto integerPtr = reinterpret_cast<std::uintptr_t>(myInteger);

	auto region = memoryManager.getLayout().findRegion(integerPtr);
	assert(region != nullptr);
	auto sameRegion = memoryManager.getLayout().findRegion(region->getAddress());
	assert(region == sameRegion);

	std::cout << "Page region: " << region->getAddress() << "-" << region->getAddress() + region->getLength() << std::endl;

	std::cout << std::dec;

	int val;
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "Before writing: " << val << std::endl;
	val = 123;
	memoryManager.write(integerPtr, &val, sizeof(int));
	val = 1337;
	memoryManager.read(integerPtr, &val, sizeof(int));
	std::cout << "After writing: " << val << std::endl;
	assert(val == 123);

	return 0;
}