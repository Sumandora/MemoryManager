#include "MemoryManager/LinuxMemoryManager.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <print>

int main()
{
	MemoryManager::LinuxMemoryManager<true, true, true> memory_manager;

	std::uintptr_t my_integer = memory_manager.allocate(sizeof(int), "---" /*No permissions*/);
	memory_manager.update();

	for (const auto& reg : memory_manager.get_layout()) {
		auto name = reg.get_name().value_or("unnamed");
		auto path = reg.get_path().value_or("pathless");
		std::println("{:x}-{:x} {} {} {} ({})",
			reg.get_address(),
			reg.get_address() + reg.get_length(),
			reg.get_flags(),
			reg.is_shared() ? "shared" : "private",
			path,
			name);
	}

	std::println("Allocated memory at {:#x}", my_integer);

	const auto* region = memory_manager.get_layout().find_region(my_integer);
	assert(region != nullptr);
	const auto* same_region = memory_manager.get_layout().find_region(region->get_address());
	assert(region == same_region);

	std::println("Page region: {:#x}-{:#x}", region->get_address(), region->get_address() + region->get_length());

	auto span = region->view();

	std::uint64_t s = 0;
	for (const std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 0);

	int val = -1;
	memory_manager.read(my_integer, &val, sizeof(int));
	std::println("Before writing: {}", val);
	val = 123;
	memory_manager.write(my_integer, &val, sizeof(int));
	val = -1; // Write some garbage to it, to prevent the read function from cheating
	memory_manager.read(my_integer, &val, sizeof(int));
	std::println("After writing: {}", val);
	assert(val == 123);

	span = region->view(true);
	s = 0;
	for (const std::byte b : span)
		s += static_cast<uint8_t>(b);
	assert(s == 123);

	memory_manager.deallocate(my_integer, sizeof(int));

	return 0;
}
