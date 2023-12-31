#include "../Include/MemoryManager/ExternalMemoryManager.hpp"

#include <fcntl.h>
#include <fstream>
#include <cstring>
#include <limits>
#include <sstream>
#include <unistd.h>
#include <utility>

using namespace MemoryManager;

constexpr int invalidFileHandle = -1;

static int openFileHandle(const std::string& pid, ExternalMemoryManager::Mode mode) {
	using Mode = ExternalMemoryManager::Mode;
	if(mode == Mode::NONE)
		return invalidFileHandle;

	int flag;
	switch (mode) {
	case Mode::READ:
		flag = O_RDONLY;
		break;
	case Mode::WRITE:
		flag = O_WRONLY;
		break;
	case Mode::READ_AND_WRITE:
		flag = O_RDWR;
		break;
	default:
		return invalidFileHandle;
	}

	return open(("/proc/" + pid + "/mem").c_str(), flag);
}

ExternalMemoryManager::ExternalMemoryManager(std::size_t pid, Mode mode)
	: ExternalMemoryManager(std::to_string(pid), mode)
{
}

ExternalMemoryManager::ExternalMemoryManager(std::string pid, Mode mode)
	: pid(std::move(pid))
	, mode(mode)
	, memInterface(openFileHandle(this->pid, mode))
{
#ifndef EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS
	if(mode != Mode::NONE && memInterface == -1)
		throw std::runtime_error(strerror(errno));
#endif
}

ExternalMemoryManager::~ExternalMemoryManager() {
	close(memInterface);
}

bool ExternalMemoryManager::isRead() const
{
	return mode == Mode::READ || mode == Mode::READ_AND_WRITE;
}

bool ExternalMemoryManager::isWrite() const
{
	return mode == Mode::WRITE || mode == Mode::READ_AND_WRITE;
}

const MemoryLayout* ExternalMemoryManager::getLayout() const
{
	return &this->layout;
}

void ExternalMemoryManager::update()
{
	std::fstream fileStream{ "/proc/" + pid + "/maps", std::fstream::in };
	if (!fileStream) {
		layout.clear();
	}

	decltype(layout) newLayout{};
	for (std::string line; std::getline(fileStream, line);) {
		if (line.empty())
			continue; // ?

		std::stringstream ss{ line };

		std::uint64_t begin;
		std::uint64_t end;
		std::array<char, 4> permissions{};

		ss >> std::hex >> begin;
		ss.ignore(1);
		ss >> end;
		ss.ignore(1);
		ss.read(permissions.begin(), 4);
		ss.ignore(std::numeric_limits<std::streamsize>::max(), '/');

		std::optional<std::string> name;
		if(!ss.eof()) {
			name = {'/'};
			std::string token;
			while (!ss.eof()) {
				ss >> token;
				if (token == "(deleted)")
					break;
				*name += token + " ";
			}
		}

		newLayout.insert(MemoryRegion{ this, begin, end - begin, Flags{ permissions }, name });
	}

	fileStream.close();

	layout = newLayout;
}

void ExternalMemoryManager::read(std::uintptr_t address, void* content, std::size_t length) const
{
	if (!isRead()) {
#ifndef EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS
		throw std::logic_error("Read is disabled");
#else
		return;
#endif
	}

	std::ignore = pread(memInterface, content, length, static_cast<off_t>(address));
}

void ExternalMemoryManager::write(std::uintptr_t address, const void* content, std::size_t length) const
{
	if (!isWrite()) {
#ifndef EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS
		throw std::logic_error("Write is disabled");
#else
		return;
#endif
	}

	std::ignore = pwrite(memInterface, content, length, static_cast<off_t>(address));
}

// TODO: This is possible with remote function calls
void* ExternalMemoryManager::allocate(std::uintptr_t address, std::size_t size, int protection) const
{
#ifndef EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS
	throw std::runtime_error("allocate() is unsupported");
#endif
	return nullptr;
}

void ExternalMemoryManager::deallocate(std::uintptr_t address, std::size_t size) const
{
#ifndef EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS
	throw std::runtime_error("deallocate() is unsupported");
#endif
}