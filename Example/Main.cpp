#include <cassert>
#include <iostream>
#include <sys/mman.h>

#define MEMORYMANAGER_DEFINE_PTR_WRAPPER
#include "MemoryManager/LocalMemoryManager.hpp"

int main()
{
	MemoryManager::MemoryManager* memoryManager = new MemoryManager::LocalMemoryManager { MemoryManager::LocalMemoryManager::Mode::NONE };

	int* myInteger = (int*)mmap(nullptr, sizeof(int), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	memoryManager->update();

	std::cout << std::hex;
	for(const MemoryManager::MemoryRegion& reg : *memoryManager->getLayout()) {
		(std::cout << (reg.name.has_value() ? reg.name.value() : std::string("(empty)")) << ' ' << reg.begin() << '-' << reg.end() << std::endl);
	}

	std::cout << "Allocated memory at " << myInteger << std::endl;

	auto region = memoryManager->getLayout()->findRegion(reinterpret_cast<std::uintptr_t>(myInteger) + sizeof(int) / 2 /* Prevent us from cheating */);
	assert(region != nullptr);
	auto sameRegion = memoryManager->getLayout()->findRegion(region->begin());
	assert(region == sameRegion);

	std::cout << "Page region: " << static_cast<std::uintptr_t>(region->begin()) << "-" << static_cast<std::uintptr_t>(region->end()) << std::endl;

	std::cout << std::dec;

	std::cout << "Before writing: " << memoryManager->getPointer(myInteger).read<int>() << std::endl;
	memoryManager->getPointer(myInteger).write(123);
	std::cout << "After writing: " << memoryManager->getPointer(myInteger).read<int>() << std::endl;
	assert(memoryManager->getPointer(myInteger).read<int>() == 123);

	return 0;
}
