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

				std::uintptr_t begin, end;
				std::array<char, 4> perms{};
				std::string name;
				name.resize(line.length());

				sscanf(line.c_str(), "%zx-%zx %c%c%c%c %*x %*x:%*x %*x %255s",
					&begin, &end, &perms[0], &perms[1], &perms[2], &perms[3], name.data());

				name.resize(strlen(name.data()));

				bool special = false;

				if(!name.empty() && name[0] == '[') {
					special = true;
				}

				newLayout.emplace(this, begin, end - begin, Flags{ perms }, name.empty() ? std::nullopt : std::optional{ name }, special);
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
				if (res == -1)
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
				if (res == -1)
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
