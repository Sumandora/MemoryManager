#include <cassert>
#include <iostream>
#include <sys/mman.h>

#define MEMORYMANAGER_DEFINE_PTR_WRAPPER
#include "MemoryManager/LocalMemoryManager.hpp"

int main()
{
	MemoryManager::LocalMemoryManager memoryManager{ MemoryManager::LocalMemoryManager::Mode::READ_AND_WRITE };

	int* myInteger = reinterpret_cast<int*>(memoryManager.allocate(nullptr, sizeof(int), PROT_NONE));
	memoryManager.update();

	std::cout << std::hex;
	for(const MemoryManager::MemoryRegion& reg : *memoryManager.getLayout()) {
		std::cout << (reg.getName().has_value() ? reg.getName().value() : std::string("(empty)")) << ' ' << reg.getFlags().asString() << ' ' << reg.getBeginAddress() << '-' << reg.getEndAddress() << std::endl;
	}

	std::cout << "Allocated memory at " << myInteger << std::endl;

	auto region = memoryManager.getLayout()->findRegion(myInteger);
	assert(region != nullptr);
	auto sameRegion = memoryManager.getLayout()->findRegion(region->getBeginAddress());
	assert(region == sameRegion);

	std::cout << "Page region: " << region->getBeginAddress() << "-" << region->getEndAddress() << std::endl;

	std::cout << std::dec;

	std::cout << "Before writing: " << memoryManager.read<int>(myInteger) << std::endl;
	memoryManager.write(myInteger, 123);
	int read;
	std::cout << "After writing: " << (read = memoryManager.read<int>(myInteger)) << std::endl;
	assert(read == 123);

	return 0;
}