#ifndef MEMORYMANAGER_LOCALMEMORYMANAGER_HPP
#define MEMORYMANAGER_LOCALMEMORYMANAGER_HPP

#include "MemoryManager/ExternalMemoryManager.hpp"
#include <sys/mman.h>

namespace MemoryManager {

	template<RWMode Mode>
	class LocalMemoryManager : public ExternalMemoryManager<Mode> {
	public:
		explicit LocalMemoryManager()
			: ExternalMemoryManager<Mode>("self")
		{
		}

	protected:
		static constexpr int protectionFlagsToPosix(ProtectionFlags flags) {
			int prot = 0;

			if(flags.isReadable())
				prot |= PROT_READ;
			if(flags.isWriteable())
				prot |= PROT_WRITE;
			if(flags.isExecutable())
				prot |= PROT_EXEC;

			return prot;
		}
	public:
		[[nodiscard]] std::uintptr_t allocate(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const override
		{
			int flags = MAP_PRIVATE | MAP_ANONYMOUS;
			void* addr = reinterpret_cast<void*>(address);
			if(addr != nullptr)
				flags |= MAP_FIXED_NOREPLACE;
			void* res = mmap(addr, size, protectionFlagsToPosix(protection), flags, -1, 0);
			if(res == MAP_FAILED)
				throw std::runtime_error(strerror(errno));
			return reinterpret_cast<uintptr_t>(res);
		}
		void deallocate(std::uintptr_t address, std::size_t size) const override
		{
			int res = munmap(reinterpret_cast<void*>(address), size);

			if(res == -1)
				throw std::runtime_error(strerror(errno));
		}
		void protect(std::uintptr_t address, std::size_t size, ProtectionFlags protection) const override {
			int res = mprotect(reinterpret_cast<void*>(address), size, protectionFlagsToPosix(protection));

			if(res == -1)
				throw std::runtime_error(strerror(errno));
		}

		void read(std::uintptr_t address, void *content, std::size_t length) const override
		{
			if constexpr (Mode == RWMode::NONE || Mode == RWMode::WRITE)
				std::memcpy(content, reinterpret_cast<void*>(address), length);
			else
				ExternalMemoryManager<Mode>::read(address, content, length);
		}
		void write(std::uintptr_t address, const void *content, std::size_t length) const override
		{
			if constexpr (Mode == RWMode::NONE || Mode == RWMode::READ)
				std::memcpy(reinterpret_cast<void*>(address), content, length);
			else
				ExternalMemoryManager<Mode>::write(address, content, length);
		}

		[[nodiscard]] bool requiresPermissionsForReading() const override {
			return Mode == RWMode::NONE || Mode == RWMode::WRITE;
		}

		[[nodiscard]] bool requiresPermissionsForWriting() const override {
			return Mode == RWMode::NONE || Mode == RWMode::READ;
		}

		[[nodiscard]] bool isRemoteAddressSpace() const override {
			return Mode == RWMode::READ_AND_WRITE || Mode == RWMode::READ; // Setting this to true would indicate that memcpy would have the same effect as using read
		}

		using MemoryManager::allocate;
		using MemoryManager::deallocate;
		using MemoryManager::protect;
		using MemoryManager::read;
		using MemoryManager::write;
	};
}

#endif
