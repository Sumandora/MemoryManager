#ifndef MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP
#define MEMORYMANAGER_EXTERNALMEMORYMANAGER_HPP

#include <cstring>
#include <fcntl.h>
#include <variant>
#include <fstream>
#include <sstream>
#include <limits>

#include "MemoryManager/MemoryManager.hpp"
#include "UnException.hpp"

namespace MemoryManager {

	enum class RWMode {
		NONE,
		READ,
		WRITE,
		READ_AND_WRITE
	};

	template<RWMode Mode_  /* Should reads use the proc mem interface? */>
	class ExternalMemoryManager : public MemoryManager {
	private:
		std::string pid;

		MemoryLayout layout;

		using FileHandle = int;

		std::conditional_t<Mode_ != RWMode::NONE, FileHandle, std::monostate> memInterface;

		static constexpr int INVALID_FILE_HANDLE = -1;

		static decltype(memInterface) openFileHandle(const std::string& pid)
		{
			if constexpr (Mode_ == RWMode::NONE)
				return std::monostate{};
			else {
				int flag;
				if constexpr (Mode_ == RWMode::READ)
					flag = O_RDONLY;
				else if constexpr(Mode_ == RWMode::WRITE)
					flag = O_WRONLY;
				else if constexpr (Mode_ == RWMode::READ_AND_WRITE)
					flag = O_RDWR;

				return open(("/proc/" + pid + "/mem").c_str(), flag);
			}
		}

	public:
		static constexpr RWMode Mode = Mode_;

		explicit ExternalMemoryManager(std::size_t pid) : ExternalMemoryManager(std::to_string(pid)) {}
		explicit ExternalMemoryManager(const std::string& pid)
			: pid(pid)
			, memInterface(openFileHandle(pid))
		{
			if constexpr (Mode != RWMode::NONE)
				if(memInterface == INVALID_FILE_HANDLE)
					throw std::runtime_error(strerror(errno));
		}
		~ExternalMemoryManager() {
			if constexpr(Mode != RWMode::NONE)
				close(memInterface);
		}

		// Since we have a file handle as one of the member variables these two operations are not possible
		ExternalMemoryManager(const ExternalMemoryManager& other) = delete;
		ExternalMemoryManager& operator=(const ExternalMemoryManager& other) = delete;

		[[nodiscard]] const MemoryLayout& getLayout() const override {
			return layout;
		}

		void update() override {
			std::fstream fileStream{ "/proc/" + pid + "/maps", std::fstream::in };
			if (!fileStream) {
				throw std::exception{};
			}

			MemoryLayout newLayout{};
			for (std::string line; std::getline(fileStream, line);) {
				if (line.empty())
					continue; // ?

				std::stringstream ss{ line };

				std::uintptr_t begin;
				std::uintptr_t end;
				std::array<char, 4> permissions{};

				ss >> std::hex >> begin;
				ss.ignore(1); // space
				ss >> end >> std::dec;
				ss.ignore(1); // space
				ss.read(permissions.begin(), 4);
				ss.ignore(1 /*space*/ + 8 /*offset*/ + 1 /*space*/ + 2 /*major dev*/ + 1 /*colon*/ + 2 /*minor dev*/ + 1 /*space*/);
				ss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // skip over inode
				// skip over spaces
				while(!ss.eof() && ss.peek() == ' ')
					ss.ignore(1);

				std::optional<std::string> name = std::nullopt;
				bool special = false;
				if (!ss.eof()) {
					name = { static_cast<char>(ss.get()) };
					if ((*name)[0] == '[') {
						special = true;
					}
					std::string token;
					while (!ss.eof()) {
						ss >> token;
						if (token.empty() || token == "(deleted)")
							break;
						*name += token + " ";
					}
					name->pop_back(); // the space
				}

				newLayout.emplace(this, begin, end - begin, Flags{ permissions }, name, special);
			}

			fileStream.close();

			layout = std::move(newLayout);
		}

		[[nodiscard]] std::size_t getPageGranularity() const override {
			// The page size could, in theory, be a different one for each process, but under Linux that doesn't happen to my knowledge.
			static std::size_t pageSize = getpagesize();
			return pageSize;
		}

		[[nodiscard]] std::uintptr_t allocate(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const override {
			(void)address;
			(void)size;
			(void)protection;
			throw UnException::UnsupportedOperationException{};
		}
		void deallocate(std::uintptr_t address, std::size_t size) const override {
			(void)address;
			(void)size;
			throw UnException::UnsupportedOperationException{};
		}
		void protect(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const override {
			(void)address;
			(void)size;
			(void)protection;
			throw UnException::UnsupportedOperationException{};
		}

		void read(std::uintptr_t address, void *content, std::size_t length) const override {
			if constexpr(Mode != RWMode::READ && Mode != RWMode::READ_AND_WRITE)
				throw UnException::UnimplementedException{};
			else {
#ifdef __GLIBC__
				auto res = pread64(memInterface, content, length, static_cast<off64_t>(address));
#else
				auto res = pread(memInterface, content, length, static_cast<off_t>(address));
#endif
				if (res != length)
					throw std::runtime_error(strerror(errno));
			}
		}
		void write(std::uintptr_t address, const void *content, std::size_t length) const override {
			if constexpr(Mode != RWMode::WRITE && Mode != RWMode::READ_AND_WRITE)
				throw UnException::UnimplementedException{};
			else {
#ifdef __GLIBC__
				auto res = pwrite64(memInterface, content, length, static_cast<off64_t>(address));
#else
				auto res = pwrite(memInterface, content, length, static_cast<off_t>(address));
#endif
				if (res != length)
					throw std::runtime_error(strerror(errno));
			}
		}

		[[nodiscard]] bool requiresPermissionsForReading() const override {
			return false;
		}

		[[nodiscard]] bool requiresPermissionsForWriting() const override {
			return false;
		}

		[[nodiscard]] bool isRemoteAddressSpace() const override {
			return true;
		}

		using MemoryManager::allocate;
		using MemoryManager::deallocate;
		using MemoryManager::protect;
		using MemoryManager::read;
		using MemoryManager::write;
	};
}

#endif
