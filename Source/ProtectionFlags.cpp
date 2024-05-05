#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;

ProtectionFlags::ProtectionFlags(std::array<char, 3> permissions) {
	set(0, permissions.at(0) == 'r');
	set(1, permissions.at(1) == 'w');
	set(2, permissions.at(2) == 'x');
}

ProtectionFlags::ProtectionFlags(bool readable, bool writable, bool executable) {
	set(0, readable);
	set(1, writable);
	set(2, executable);
}
