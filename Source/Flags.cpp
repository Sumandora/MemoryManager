#include "MemoryManager/MemoryManager.hpp"

using namespace MemoryManager;

Flags::Flags(std::array<char, 4> permissions)
{
	set(0, permissions.at(0) == 'r');
	set(1, permissions.at(1) == 'w');
	set(2, permissions.at(2) == 'x');
	set(3, permissions.at(3) == 'p');
}

Flags::Flags(bool readable, bool writable, bool executable, bool _private) {
	set(0, readable);
	set(1, writable);
	set(2, executable);
	set(3, _private);
}

std::string Flags::asString() const
{
	std::string string;
	string += test(0) ? 'r' : '-';
	string += test(1) ? 'w' : '-';
	string += test(2) ? 'x' : '-';
	string += test(3) ? 'p' : '-';
	return string;
}