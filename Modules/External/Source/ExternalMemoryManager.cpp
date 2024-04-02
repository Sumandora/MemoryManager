#include "MemoryManager/ExternalMemoryManager.hpp"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <unistd.h>

#include "UnException.hpp"

using namespace MemoryManager;

constexpr int invalidFileHandle = -1;

static int openFileHandle(const std::string& pid, ExternalMemoryManager::Mode mode)
{
	int flag;
	switch (mode) {
	using Mode = ExternalMemoryManager::Mode;
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

ExternalMemoryManager::ExternalMemoryManager(const std::string& pid, Mode mode)
	: pid(pid)
	, mode(mode)
	, memInterface(openFileHandle(pid, mode))
{
	if (mode != Mode::NONE && memInterface == invalidFileHandle)
		throw std::runtime_error(strerror(errno));
}

ExternalMemoryManager::~ExternalMemoryManager()
{
	close(memInterface);
}

bool ExternalMemoryManager::doesRead() const
{
	return mode == Mode::READ || mode == Mode::READ_AND_WRITE;
}

bool ExternalMemoryManager::doesWrite() const
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

		std::uintptr_t begin;
		std::uintptr_t end;
		std::array<char, 4> permissions{};

		ss >> std::hex >> begin;
		ss.ignore(1);
		ss >> end;
		ss.ignore(1);
		ss.read(permissions.begin(), 4);
		ss.ignore(std::numeric_limits<std::streamsize>::max(), '/');

		std::optional<std::string> name;
		if (!ss.eof()) {
			name = { '/' };
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

std::size_t ExternalMemoryManager::getPageGranularity() const
{
	return getpagesize(); // The page size could in theory be a different one for each process, but under Linux that doesn't happen to my knowledge.
}

void ExternalMemoryManager::read(std::uintptr_t address, void* content, std::size_t length) const
{
	if (!this->doesRead()) {
		throw UnException::UnsupportedOperationException{};
	}

	// Magic
#ifdef __GLIBC__
	long res = pread64(memInterface, content, length, static_cast<off64_t>(address));
#else
	long res = pread(memInterface, content, length, static_cast<off_t>(address));
#endif
	if (res != length)
		throw std::runtime_error(strerror(errno));
}

void ExternalMemoryManager::write(std::uintptr_t address, const void* content, std::size_t length) const
{
	if (!this->doesWrite()) {
		throw UnException::UnsupportedOperationException{};
	}

	// Magic
	#ifdef __GLIBC__
	long res = pwrite64(memInterface, content, length, static_cast<off64_t>(address));
	#else
	long res = pwrite(memInterface, content, length, static_cast<off_t>(address));
	#endif
	if (res != length)
		throw std::runtime_error(strerror(errno));
}

// TODO: This is possible with remote function calls
std::uintptr_t ExternalMemoryManager::allocate(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const
{
	throw UnException::UnimplementedException{};
}

void ExternalMemoryManager::deallocate(std::uintptr_t address, std::size_t size) const
{
	throw UnException::UnimplementedException{};
}

void ExternalMemoryManager::protect(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const
{
	throw UnException::UnimplementedException{};
}